<?php

/**
 * implementation of hook_pre_provision_install
 */
function drush_civicrm_pre_provision_install($url = NULL) {
  /*
  if (d()->type == 'site') {
    // if necessary ...
  }
  */
}

/**
 * implementation of hook_post_provision_install
 */
function drush_civicrm_post_provision_install($url = null) {
  if (d()->type == 'site') {
    // Check if the CiviCRM code base is present in the platform
    // Note: after putting the code there, you need to verify
    // the platform.
    $packages = drush_get_option('packages', 'platform');

    if (!array_key_exists('civicrm', $packages['base']['modules'])) {
      return;
    }

    drush_log(dt("Configuring site for civicrm support"));

    $db_user = drush_get_option('db_user', 'site');
    $db_passwd = drush_get_option('db_passwd', 'site');
    $db_host = drush_get_option('db_host', 'site');
    $db_name = drush_get_option('db_name', 'site');

    drush_set_option('dbuser', $db_user);
    drush_set_option('dbpass', $db_passwd);
    drush_set_option('dbhost', $db_host);
    drush_set_option('dbname', $db_name);

    $sitekey = md5(uniqid( '', true ) . $siteRoot);

    drush_set_option('sitekey', $sitekey);
    drush_set_option('civicrm_sitekey', $sitekey, 'site');

    // Generates the civicrm.settings.php
    civicrm_pkg_install();

    // Install the DB, not sure why the drush module is not doing it
    global $crmPath;

    $dsn = "mysql://{$db_user}:{$db_passwd}@{$db_host}/{$db_name}?new_link=true";
    $drupalRoot = drush_get_context('DRUSH_DRUPAL_ROOT');
    $crmPath = $drupalRoot . '/sites/all/modules/civicrm'; // FIXME
    $sqlPath = $crmPath . DIRECTORY_SEPARATOR . 'sql';

    // include civicrm installer helper file (otherwise PEAR DB.php will fail to include parts)
    $include_path = $crmPath . "/packages/:" . get_include_path( );
    set_include_path( $include_path );

    $civicrmInstallerHelper = $crmPath . "/install/civicrm.php";
    if ( !file_exists($civicrmInstallerHelper) ) {
        drush_die(dt("CiviCRM installer helper file is missing."));
    }
    require_once "$civicrmInstallerHelper";

    civicrm_source($dsn, $sqlPath . DIRECTORY_SEPARATOR . 'civicrm.mysql');
    civicrm_source($dsn, $sqlPath . DIRECTORY_SEPARATOR . 'civicrm_data.mysql');
    civicrm_source($dsn, $sqlPath . DIRECTORY_SEPARATOR . 'civicrm_acl.mysql');

    // Create a civicron user and role
    $url = drush_get_option('uri', false);

    $civicron_cronpassword = md5(uniqid(rand(), true));

    drush_set_option('mail', 'civicrm-no-reply@' . $url);
    drush_set_option('password', $civicron_password);
    drush_user_create('civicron');

    drush_set_option('civicrm_cron_username', 'civicron', 'site');
    drush_set_option('civicrm_cron_password', $civicron_password, 'site');

    // Create civicron role
    db_query("INSERT INTO {role} (name) VALUES ('%s')", 'civicron');

    drush_user_add_role('civicron', 'civicron');

    // Assign permissions to civicron role
    db_query("INSERT INTO {permission} (rid, perm)
              VALUES (3, 'access CiviCRM, access CiviContribute, access CiviEvent, access CiviMail, access CiviMail subscribe/unsubscribe pages, access CiviMember, access CiviPledge, access CiviReport, access Contact Dashboard, access Report Criteria, access all custom data, add contacts, edit all contacts, edit all events, edit contributions, edit event participants, view all activities, view all contacts, view all notes')");

    drush_pm_enable('civicrm');
  }
}
