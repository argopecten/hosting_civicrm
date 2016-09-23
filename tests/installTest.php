<?php

namespace HostingCiviTest;

/**
 * Tests that phpunit is working.
 */
class installTest extends HostingCiviTestCase {
  /**
   * @param string|null $name
   */
  public function __construct($name = NULL) {
    parent::__construct($name);
  }

  /**
   * Called before all test functions will be executed.
   * this function is defined in PHPUnit_TestCase and overwritten
   * here.
   *
   * https://phpunit.de/manual/current/en/fixtures.html#fixtures.more-setup-than-teardown
   */
  public static function setUpBeforeClass() {
    Command\PlatformInstall::run('civicrm43d7');
    Command\PlatformInstall::run('civicrm44d7');
    Command\PlatformInstall::run('civicrm46d7');
    Command\PlatformInstall::run('civicrm46d6');
    Command\PlatformInstall::run('civicrm47d7');
  }

  /**
   * Called after the test functions are executed.
   * this function is defined in PHPUnit_TestCase and overwritten
   * here.
   */
  public static function tearDownAfterClass() {
    Command\PlatformDelete::run('civicrm43d7');
    Command\PlatformDelete::run('civicrm44d7');
    Command\PlatformDelete::run('civicrm46d7');
    Command\PlatformDelete::run('civicrm46d6');
    Command\PlatformDelete::run('civicrm47d7');
  }

  /**
   * Test the toString function.
   */
  public function testHello() {
    $result = 'hello';
    $expected = 'hello';
    $this->assertEquals($result, $expected);
  }

  /**
   * Test the installation and deletion of sites with CiviCRM 4.3 D7.
   */
  public function testInstallAndDelete43d7() {
    Command\SiteInstall::run('civicrm43d7', 'civicrm43d7-standard');
    Command\SiteDelete::run('civicrm43d7-standard');
  }

  /**
   * Test the installation and deletion of sites with CiviCRM 4.4 D7.
   */
  public function testInstallAndDelete44d7() {
    Command\SiteInstall::run('civicrm44d7', 'civicrm44d7-standard');
    Command\SiteDelete::run('civicrm44d7-standard');
  }

  /**
   * Test the installation and deletion of sites with CiviCRM 4.6 D7.
   */
  public function testInstallAndDelete46d7() {
    Command\SiteInstall::run('civicrm46d7', 'civicrm46d7-standard');
    Command\SiteDelete::run('civicrm46d7-standard');
  }

  /**
   * Test the installation and deletion of sites with CiviCRM 4.6 D6.
   */
  public function testInstallAndDelete46d6() {
    Command\SiteInstall::run('civicrm46d6', 'civicrm46d6-default', 'default');
    Command\SiteDelete::run('civicrm46d6-default');
  }

  /**
   * Test the installation and deletion of sites with CiviCRM 4.7 D7.
   */
  public function testInstallAndDelete47d7() {
    Command\SiteInstall::run('civicrm47d7', 'civicrm47d7-standard', 'default');
    Command\SiteDelete::run('civicrm47d7-default');
  }

}
