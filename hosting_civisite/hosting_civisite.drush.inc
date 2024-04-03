<?php

/**
 * @file
 * Drush hooks for the Hosting civisite module.
 */

/**
 * Implements hook_hosting_TASK_OBJECT_context_options().
 */
function hosting_hosting_civisite_context_options(&$task) {
  $task->context_options['db_server'] = hosting_context_name($task->ref->db_server);
  $task->context_options['civiplatform'] = hosting_context_name($task->ref->civiplatform);

  $task->context_options['uri'] = strtolower(trim($task->ref->title));
  $task->context_options['language'] = $task->ref->civisite_language;

  $task->context_options['type'] = $task->ref->type;

  // $profile = node_load($task->ref->profile);
  // $task->context_options['profile'] = $profile->short_name;

  if ($task->task_type == 'civicrm-install') {
    $client = node_load($task->ref->client);
    $user = user_load($task->uid);
    $task->options['client_email'] = $user->mail;
    $task->context_options['client_name'] = $client->uname;
  }
}

/**
 * Implements hook_drush_context_import().
 */
function hosting_civisite_drush_context_import($context, &$node) {
  if ($context->type == 'civisite') {
    $node->title = strtolower(trim($context->uri)); // force lowercase for existing uri
    $node->civisite_language = $context->language;
    $node->cron_key = $context->cron_key;
    $node->db_server = hosting_drush_import($context->db_server->name);
    $node->civiplatform = hosting_drush_import($context->civiplatform->name);

    // TODO: abstract this to remove duplication with import post hooks.
/* FIXME
    $profile = hosting_package_instance_load(array(
      'i.rid' => $node->civiplatform,
      'p.short_name' => $context->profile,
    ));

    if (!$profile) {
      // For D6 to D7 upgrades, try changing default to standard.
      if (!empty($context->profile) && $context->profile == 'default') {
        $profile = hosting_package_instance_load(array(
          'i.rid' => $node->civiplatform,
          'p.short_name' => 'standard',
        ));
        if ($profile) {
           drush_log(dt("Could not find the Profile !profile for Platfrom ID !platform, substituted standard", array('!profile' => $context->profile, '!platform' => $node->platform)), 'warning');
        }
      }
    }
*/

    // Log an error if there is still no profile.
    if (!$profile) {
      drush_set_error("HOSTING_CIVISITE_IMPORT_ERROR", dt("Could not find the Profile !profile for Platfrom ID !platform", array('!profile' => $context->profile, '!platform' => $node->platform)));
    }
    $node->profile = $profile->package_id;

    $client = node_load(HOSTING_DEFAULT_CLIENT);
    if ($context->client_name) {
      $client = hosting_import_client($context->client_name);
    }
    $node->client = $client->nid;
  }

}

/**
 * Map values of civisite node into command line arguments
 */
function drush_hosting_civisite_pre_hosting_task($task) {
  if (empty($task->ref->type) || $task->ref->type != 'civisite') {
    return;
  }

  $task = &drush_get_context('HOSTING_TASK');

  // Add support for aliases and redirections
  if (module_exists('hosting_alias')) {
    $aliases = implode(',', hosting_alias_get_aliases($task->ref));
    $task->context_options['aliases'] = ($aliases) ? $aliases : 'null';

    $redirection = db_query("SELECT redirection FROM {hosting_site_alias} WHERE vid = :vid", array(':vid' => $task->ref->vid))->fetchField();
    $task->context_options['redirection'] = ($redirection) ? $redirection : 'null';
  }

  // FIXME The rest is based on hosting_site, but not implemented yet.
  return;

  if ($task->task_type == 'restore') {
    $backup = hosting_civisite_get_backup($task->task_args['bid']);
    $task->args[1] = $backup['filename'];
  }

  if ($task->task_type == 'backup-delete') {
    foreach ($task->task_args as $bid => $filename) {
      if ($filename !== '0') {
        $backups[] = $filename;
      }
    }
    $task->args[1] = implode(',', $backups);
  }
}

/**
 * implementation of the hosting_post_install hook
 */
function hosting_civisite_post_hosting_civicrm_install_task($task, $data) {
  if ($task->ref->type == 'civisite') {
    $context = $data['context'];

    $task->ref->civisite_status = HOSTING_CIVISITE_ENABLED;
    $task->ref->no_verify = TRUE;
    $task->ref->verified = REQUEST_TIME;
    $task->ref->db_name = $data['self']['db_name'];
    if ($context['cron_key']) {
      $task->ref->cron_key = $context['cron_key'];
    }
    node_save($task->ref);

/* FIXME: there is on easy way (API) to get a one-time login link.
    if ($context['login_link']) {
      drush_log($context['login_link']);
      // we need to store the timestamp too because expire only means it can be deleted
      // after an amount of time, not will be deleted.
      $cache = array(
        'expire' => strtotime("+24 hours"),
        'link' => $context['login_link'],
      );
      cache_set('hosting:civisite:' . $task->ref->nid . ':login_link', $cache, 'cache', $cache['expire'] );
    }
*/
  }
}

/**
 * Implements the hosting_failed_install hook
 */
function hosting_civisite_hosting_civicrm_install_task_rollback($task, $data) {
  // FIXME: this code was copy-pasted. Do we have an equivalent way to check?
  if (drush_cmp_error('PROVISION_DRUPAL_SITE_INSTALLED')) {
    // Site has already been installed. Try to import instead.
    drush_log(dt("This civisite appears to be installed already. Generating an import task."));
    hosting_add_task($task->rid, 'import');
  }
  else {
    $task->ref->no_verify = TRUE;
    $task->ref->civisite_status = HOSTING_CIVISITE_DISABLED;
    node_save($task->ref);
  }
}

/**
 * Implements hook_hosting_post_disable().
 * I am not very fond of the use of bitwise operators with negatives.
 */
function hosting_civisite_post_hosting_civicrm_disable_task($task, $data) {
  if ($data['context']['backup_file'] && $data->ref->type == 'civisite') {
    $civiplatform = node_load($task->ref->civiplatform);
    hosting_civisite_add_backup($task->ref->nid, $civiplatform->web_server, $data['context']['backup_file'], t('Generated before being disabled'), $data['context']['backup_file_size']);
  }
  $task->ref->civisite_status = HOSTING_CIVISITE_DISABLED;
  $task->ref->no_verify = TRUE;
  node_save($task->ref);

  // Record the backup created during disable.
  $task->task_args['description'] = t('Pre-disable backup');
  hosting_civisite_post_hosting_backup_task($task, $data);
}

/**
 * Implements hook_hosting_post_restore().
 * I am not very fond of the use of bitwise operators with negatives.
 */
function hosting_civisite_post_hosting_civicrm_restore_task($task, $data) {
  if ($data['context']['backup_file'] && $task->ref->type == 'civisite') {
    $civiplatform = node_load($task->ref->civiplatform);
    hosting_civisite_add_backup($task->ref->nid, $civiplatform->web_server, $data['context']['backup_file'], t('Generated before being restored to a previous version'), $data['context']['backup_file_size']);
  }
}

/**
 * Implements hook_hosting_post_enable().
 */
function hosting_civisite_post_hosting_civicrm_enable_task($task, $data) {
  $task->ref->civisite_status = HOSTING_CIVISITE_ENABLED;
  $task->ref->no_verify = TRUE;
  node_save($task->ref);
}

/**
 * Implements hook_hosting_post_DELETE().
 */
function hosting_civisite_post_hosting_civicrm_delete_task($task, $data) {
  if ($task->ref->type == 'civisite') {
    $task->ref->civisite_status = HOSTING_CIVISITE_DELETED;
    $task->ref->no_verify = TRUE;
    hosting_context_delete($task->ref->nid);
    node_save($task->ref);

    // Record the backup created during delete.
    $task->task_args['description'] = t('Pre-delete backup');
    hosting_civisite_post_hosting_backup_task($task, $data);

    // Clean up package instances
    # TODO: not supported for now (we do not manage civi themes/plugins)
    # hosting_package_instance_sync($task->ref->nid, 'civisite');
  }
}

/**
 * Implements hook_hosting_post_backup().
 *
 * Adds the data file that was saved to the civisite backup history.
 * This is needed to be able to restore.
 */
function hosting_civisite_post_hosting_civicrm_backup_task($task, $data) {
  if ($data['context']['backup_file'] && $task->ref->type == 'civisite') {
    $civiplatform = node_load($task->ref->civiplatform);

    $desc = $task->task_args['description'];
    $desc = ($desc) ? $desc : t('Generated on request');
    hosting_civisite_add_backup($task->ref->nid, $civiplatform->web_server, $data['context']['backup_file'], $desc, $data['context']['backup_file_size']);
  }
}


/**
 * Implements hook_post_hosting_import_task().
 */
function hosting_civisite_post_hosting_civicrm_import_task($task, $data) {
  hosting_import_civisite($task->ref->nid, $data['context'], $task->ref->civiplatform);

  $task->ref->db_name = $data['self']['db_name'];
  node_save($task->ref);

  $packages = $data['context']['packages'];
  hosting_package_sync($packages);
  hosting_package_instance_sync($task->ref->nid, $task->ref->type, $packages);

}

/**
 * Implements hook_post_hosting_verify_task().
 */
function hosting_civisite_post_hosting_civicrm_verify_task($task, $data) {
  if ($task->ref->type == 'civisite') {
    $task->ref->verified = REQUEST_TIME;
    $task->ref->no_verify = TRUE;
    $task->ref->db_name = $data['self']['db_name'];
    if ($data['context']['cron_key']) {
      $task->ref->cron_key = $data['context']['cron_key'];
    }
    node_save($task->ref);

    $context = $data['context'];
    // $packages = $context['packages'];
    // hosting_package_sync($packages);
    // hosting_package_instance_sync($task->ref->nid, $task->ref->type, $packages);
  }
}

/**
 * Implements the hosting_failed_install hook
 */
function hosting_civisite_hosting_verify_task_rollback($task, $data) {
  if ($task->ref->type == 'civisite') {
    $task->ref->no_verify = TRUE;
    $task->ref->verified = 0;
    node_save($task->ref);
  }
}

/**
 * Implements hook_hosting_import_task_rollback().
 */
function hosting_civisite_hosting_import_task_rollback($task, $data) {
  if ($task->ref->type == 'civisite') {
    $task->ref->no_verify = TRUE;
    $task->ref->verified = 0;
    $task->ref->civisite_status = HOSTING_CIVISITE_DISABLED;
    node_save($task->ref);
  }
}

/**
 * Implements hook_post_hosting_login_reset_task().
 */
function hosting_civisite_post_hosting_login_reset_task($task, $data) {
  if ($task->ref->type == 'civisite') {
    $task->ref->no_verify = TRUE;
    node_save($task->ref);

    $context = $data['context'];
    if ($context['login_link']) {
      // we need to store the timestamp too because expire only means it can be deleted
      // after an amount of time, not will be deleted.
      $cache = array(
        'expire' => strtotime("+24 hours"),
        'link' => $context['login_link'],
      );
      cache_set('hosting:civisite:' . $task->ref->nid . ':login_link', $cache, 'cache', $cache['expire'] );
    }
  }
}

/**
 * Implements the hosting_post_backup_delete hook
 */
function hosting_civisite_post_hosting_backup_delete_task($task, $data) {
  if ($task->ref->type == 'civisite') {
    foreach ($task->task_args as $bid => $filename) {
      if ($filename !== '0') {
        hosting_civisite_delete_backup($bid);
      }
    }
  }
}

/**
 * Implements hook_hosting_post_backup().
 *
 * Adds the data file that was saved to the site backup history.
 * This is needed to be able to restore.
 */
function hosting_civisite_post_hosting_backup_task($task, $data) {
  if ($data['context']['backup_file'] && $task->ref->type == 'civisite') {
    $platform = node_load($task->ref->civiplatform);

    $desc = $task->task_args['description'];
    $desc = ($desc) ? $desc : t('Generated on request');
    hosting_site_add_backup($task->ref->nid, $platform->web_server, $data['context']['backup_file'], $desc, $data['context']['backup_file_size']);
  }
}