<?php
/**
 * A File upgrader class for WordPress.
 *
 * This set of classes are designed to be used to upgrade/install a local set of files on the filesystem via the Filesystem Abstraction classes.
 *
 * @link http://trac.wordpress.org/ticket/7875 consolidate plugin/theme/core upgrade/install functions
 *
 * @package WordPress
 * @subpackage Upgrader
 * @since 2.8.0
 */

/**
 * WordPress Upgrader class for Upgrading/Installing a local set of files via the Filesystem Abstraction classes from a Zip file.
 *
 * @TODO More Detailed docs, for methods as well.
 *
 * @package WordPress
 * @subpackage Upgrader
 * @since 2.8.0
 */
class WP_Upgrader {
	var $strings = array();
	var $skin = null;
	var $result = array();

	function WP_Upgrader($skin = null) {
		return $this->__construct($skin);
	}
	function __construct($skin = null) {
		if ( null == $skin )
			$this->skin = new WP_Upgrader_Skin();
		else
			$this->skin = $skin;
	}

	function init() {
		$this->skin->set_upgrader($this);
		$this->generic_strings();
	}

	function generic_strings() {
		$this->strings['bad_request'] = __('Invalid Data provided.');
		$this->strings['fs_unavailable'] = __('Could not access filesystem.');
		$this->strings['fs_error'] = __('Filesystem error');
		$this->strings['fs_no_root_dir'] = __('Unable to locate WordPress Root directory.');
		$this->strings['fs_no_content_dir'] = __('Unable to locate WordPress Content directory (wp-content).');
		$this->strings['fs_no_plugins_dir'] = __('Unable to locate WordPress Plugin directory.');
		$this->strings['fs_no_themes_dir'] = __('Unable to locate WordPress Theme directory.');
		$this->strings['fs_no_folder'] = __('Unable to locate needed folder (%s).');

		$this->strings['download_failed'] = __('Download failed.');
		$this->strings['installing_package'] = __('Installing the latest version.');
		$this->strings['folder_exists'] = __('Destination folder already exists.');
		$this->strings['mkdir_failed'] = __('Could not create directory.');
		$this->strings['bad_package'] = __('Incompatible Archive');

		$this->strings['maintenance_start'] = __('Enabling Maintenance mode.');
		$this->strings['maintenance_end'] = __('Disabling Maintenance mode.');
	}

	function fs_connect( $directories = array() ) {
		global $wp_filesystem;

		if ( false === ($credentials = $this->skin->request_filesystem_credentials()) )
			return false;

		if ( ! WP_Filesystem($credentials) ) {
			$error = true;
			if ( is_object($wp_filesystem) && $wp_filesystem->errors->get_error_code() )
				$error = $wp_filesystem->errors;
			$this->skin->request_filesystem_credentials($error); //Failed to connect, Error and request again
			return false;
		}

		if ( ! is_object($wp_filesystem) )
			return new WP_Error('fs_unavailable', $this->strings['fs_unavailable'] );

		if ( is_wp_error($wp_filesystem->errors) && $wp_filesystem->errors->get_error_code() )
			return new WP_Error('fs_error', $this->strings['fs_error'], $wp_filesystem->errors);

		foreach ( (array)$directories as $dir ) {
			if ( ABSPATH == $dir && ! $wp_filesystem->abspath() )
				return new WP_Error('fs_no_root_dir', $this->strings['fs_no_root_dir']);

			elseif ( WP_CONTENT_DIR == $dir && ! $wp_filesystem->wp_content_dir() )
				return new WP_Error('fs_no_content_dir', $this->strings['fs_no_content_dir']);

			elseif ( WP_PLUGIN_DIR == $dir && ! $wp_filesystem->wp_plugins_dir() )
				return new WP_Error('fs_no_plugins_dir', $this->strings['fs_no_plugins_dir']);

			elseif ( WP_CONTENT_DIR . '/themes' == $dir && ! $wp_filesystem->find_folder(WP_CONTENT_DIR . '/themes') )
				return new WP_Error('fs_no_themes_dir', $this->strings['fs_no_themes_dir']);

			elseif ( ! $wp_filesystem->find_folder($dir) )
				return new WP_Error('fs_no_folder', sprintf($strings['fs_no_folder'], $dir));
		}
		return true;
	} //end fs_connect();

	function download_package($package) {

		if ( ! preg_match('!^(http|https|ftp)://!i', $package) && file_exists($package) ) //Local file or remote?
			return $package; //must be a local file..

		if ( empty($package) )
			return new WP_Error('no_package', $this->strings['no_package']);

		$this->skin->feedback('downloading_package', $package);

		$download_file = download_url($package);

		if ( is_wp_error($download_file) )
			return new WP_Error('download_failed', $this->strings['download_failed'], $download_file->get_error_message());

		return $download_file;
	}

	function unpack_package($package, $delete_package = true) {
		global $wp_filesystem;

		$this->skin->feedback('unpack_package');

		$upgrade_folder = $wp_filesystem->wp_content_dir() . 'upgrade/';

		//Clean up contents of upgrade directory beforehand.
		$upgrade_files = $wp_filesystem->dirlist($upgrade_folder);
		if ( !empty($upgrade_files) ) {
			foreach ( $upgrade_files as $file )
				$wp_filesystem->delete($upgrade_folder . $file['name'], true);
		}

		//We need a working directory
		$working_dir = $upgrade_folder . basename($package, '.zip');

		// Clean up working directory
		if ( $wp_filesystem->is_dir($working_dir) )
			$wp_filesystem->delete($working_dir, true);

		// Unzip package to working directory
		$result = unzip_file($package, $working_dir); //TODO optimizations, Copy when Move/Rename would suffice?

		// Once extracted, delete the package if required.
		if ( $delete_package )
			unlink($package);

		if ( is_wp_error($result) ) {
			$wp_filesystem->delete($working_dir, true);
			return $result;
		}

		return $working_dir;
	}

	function install_package($args = array()) {
		global $wp_filesystem;
		$defaults = array( 'source' => '', 'destination' => '', //Please always pass these
						'clear_destination' => false, 'clear_working' => false,
						'hook_extra' => array());

		$args = wp_parse_args($args, $defaults);
		extract($args);

		@set_time_limit( 300 );

		if ( empty($source) || empty($destination) )
			return new WP_Error('bad_request', $this->strings['bad_request']);

		$this->skin->feedback('installing_package');

		$res = apply_filters('upgrader_pre_install', true, $hook_extra);
		if ( is_wp_error($res) )
			return $res;

		//Retain the Original source and destinations
		$remote_source = $source;
		$local_destination = $destination;

		$source_files = array_keys( $wp_filesystem->dirlist($remote_source) );
		$remote_destination = $wp_filesystem->find_folder($local_destination);

		//Locate which directory to copy to the new folder, This is based on the actual folder holding the files.
		if ( 1 == count($source_files) && $wp_filesystem->is_dir( trailingslashit($source) . $source_files[0] . '/') ) //Only one folder? Then we want its contents.
			$source = trailingslashit($source) . trailingslashit($source_files[0]);
		elseif ( count($source_files) == 0 )
			return new WP_Error('bad_package', $this->strings['bad_package']); //There are no files?
		//else //Its only a single file, The upgrader will use the foldername of this file as the destination folder. foldername is based on zip filename.

		//Hook ability to change the source file location..
		$source = apply_filters('upgrader_source_selection', $source, $remote_source, $this);
		if ( is_wp_error($source) )
			return $source;

		//Has the source location changed? If so, we need a new source_files list.
		if ( $source !== $remote_source )
			$source_files = array_keys( $wp_filesystem->dirlist($source) );

		//Protection against deleting files in any important base directories.
		if ( in_array( $destination, array(ABSPATH, WP_CONTENT_DIR, WP_PLUGIN_DIR, WP_CONTENT_DIR . '/themes') ) ) {
			$remote_destination = trailingslashit($remote_destination) . trailingslashit(basename($source));
			$destination = trailingslashit($destination) . trailingslashit(basename($source));
		}

		//If we're not clearing the destination folder, and something exists there allready, Bail.
		if ( ! $clear_destination && $wp_filesystem->exists($remote_destination) ) {
			$wp_filesystem->delete($remote_source, true); //Clear out the source files.
			return new WP_Error('folder_exists', $this->strings['folder_exists'], $remote_destination );
		} else if ( $clear_destination ) {
			//We're going to clear the destination if theres something there
			$this->skin->feedback('remove_old');

			$removed = true;
			if ( $wp_filesystem->exists($remote_destination) )
				$removed = $wp_filesystem->delete($remote_destination, true);

			$removed = apply_filters('upgrader_clear_destination', $removed, $local_destination, $remote_destination, $hook_extra);

			if ( is_wp_error($removed) )
				return $removed;
			else if ( ! $removed )
				return new WP_Error('remove_old_failed', $this->strings['remove_old_failed']);
		}

		//Create destination if needed
		if ( !$wp_filesystem->exists($remote_destination) )
			if ( !$wp_filesystem->mkdir($remote_destination, FS_CHMOD_DIR) )
				return new WP_Error('mkdir_failed', $this->strings['mkdir_failed'], $remote_destination);

		// Copy new version of item into place.
		$result = copy_dir($source, $remote_destination);
		if ( is_wp_error($result) ) {
			if ( $clear_working )
				$wp_filesystem->delete($remote_source, true);
			return $result;
		}

		//Clear the Working folder?
		if ( $clear_working )
			$wp_filesystem->delete($remote_source, true);

		$destination_name = basename( str_replace($local_destination, '', $destination) );
		if ( '.' == $destination_name )
			$destination_name = '';

		$this->result = compact('local_source', 'source', 'source_name', 'source_files', 'destination', 'destination_name', 'local_destination', 'remote_destination', 'clear_destination', 'delete_source_dir');

		$res = apply_filters('upgrader_post_install', true, $hook_extra, $this->result);
		if ( is_wp_error($res) ) {
			$this->result = $res;
			return $res;
		}

		//Bombard the calling function will all the info which we've just used.
		return $this->result;
	}

	function run($options) {

		$defaults = array( 	'package' => '', //Please always pass this.
							'destination' => '', //And this
							'clear_destination' => false,
							'clear_working' => true,
							'hook_extra' => array() //Pass any extra $hook_extra args here, this will be passed to any hooked filters.
						);

		$options = wp_parse_args($options, $defaults);
		extract($options);

		//Connect to the Filesystem first.
		$res = $this->fs_connect( array(WP_CONTENT_DIR, $destination) );
		if ( ! $res ) //Mainly for non-connected filesystem.
			return false;

		if ( is_wp_error($res) ) {
			$this->skin->error($res);
			return $res;
		}

		$this->skin->header();
		$this->skin->before();

		//Download the package (Note, This just returns the filename of the file if the package is a local file)
		$download = $this->download_package( $package );
		if ( is_wp_error($download) ) {
			$this->skin->error($download);
			return $download;
		}

		//Unzip's the file into a temporary directory
		$working_dir = $this->unpack_package( $download );
		if ( is_wp_error($working_dir) ) {
			$this->skin->error($working_dir);
			return $working_dir;
		}

		//With the given options, this installs it to the destination directory.
		$result = $this->install_package( array(
											'source' => $working_dir,
											'destination' => $destination,
											'clear_destination' => $clear_destination,
											'clear_working' => $clear_working,
											'hook_extra' => $hook_extra
										) );
		$this->skin->set_result($result);
		if ( is_wp_error($result) ) {
			$this->skin->error($result);
			$this->skin->feedback('process_failed');
		} else {
			//Install Suceeded
			$this->skin->feedback('process_success');
		}
		$this->skin->after();
		$this->skin->footer();
		return $result;
	}

	function maintenance_mode($enable = false) {
		global $wp_filesystem;
		$file = $wp_filesystem->abspath() . '.maintenance';
		if ( $enable ) {
			$this->skin->feedback('maintenance_start');
			// Create maintenance file to signal that we are upgrading
			$maintenance_string = '<?php $upgrading = ' . time() . '; ?>';
			$wp_filesystem->delete($file);
			$wp_filesystem->put_contents($file, $maintenance_string, FS_CHMOD_FILE);
		} else if ( !$enable && $wp_filesystem->exists($file) ) {
			$this->skin->feedback('maintenance_end');
			$wp_filesystem->delete($file);
		}
	}

}

/**
 * Plugin Upgrader class for WordPress Plugins, It is designed to upgrade/install plugins from a local zip, remote zip URL, or uploaded zip file.
 *
 * @TODO More Detailed docs, for methods as well.
 *
 * @package WordPress
 * @subpackage Upgrader
 * @since 2.8.0
 */
class Plugin_Upgrader extends WP_Upgrader {

	var $result;

	function upgrade_strings() {
		$this->strings['up_to_date'] = __('The plugin is at the latest version.');
		$this->strings['no_package'] = __('Upgrade package not available.');
		$this->strings['downloading_package'] = __('Downloading update from <span class="code">%s</span>.');
		$this->strings['unpack_package'] = __('Unpacking the update.');
		$this->strings['deactivate_plugin'] = __('Deactivating the plugin.');
		$this->strings['remove_old'] = __('Removing the old version of the plugin.');
		$this->strings['remove_old_failed'] = __('Could not remove the old plugin.');
		$this->strings['process_failed'] = __('Plugin upgrade Failed.');
		$this->strings['process_success'] = __('Plugin upgraded successfully.');
	}

	function install_strings() {
		$this->strings['no_package'] = __('Install package not available.');
		$this->strings['downloading_package'] = __('Downloading install package from <span class="code">%s</span>.');
		$this->strings['unpack_package'] = __('Unpacking the package.');
		$this->strings['installing_package'] = __('Installing the plugin.');
		$this->strings['process_failed'] = __('Plugin Install Failed.');
		$this->strings['process_success'] = __('Plugin Installed successfully.');
	}

	function install($package) {

		$this->init();
		$this->install_strings();

		$this->run(array(
					'package' => $package,
					'destination' => WP_PLUGIN_DIR,
					'clear_destination' => false, //Do not overwrite files.
					'clear_working' => true,
					'hook_extra' => array()
					));

		// Force refresh of plugin update information
		delete_transient('update_plugins');

	}

	function upgrade($plugin) {

		$this->init();
		$this->upgrade_strings();

		$current = get_transient( 'update_plugins' );
		if ( !isset( $current->response[ $plugin ] ) ) {
			$this->skin->set_result(false);
			$this->skin->error('up_to_date');
			$this->skin->after();
			return false;
		}

		// Get the URL to the zip file
		$r = $current->response[ $plugin ];

		add_filter('upgrader_pre_install', array(&$this, 'deactivate_plugin_before_upgrade'), 10, 2);
		add_filter('upgrader_clear_destination', array(&$this, 'delete_old_plugin'), 10, 4);
		//'source_selection' => array(&$this, 'source_selection'), //theres a track ticket to move up the directory for zip's which are made a bit differently, useful for non-.org plugins.

		$this->run(array(
					'package' => $r->package,
					'destination' => WP_PLUGIN_DIR,
					'clear_destination' => true,
					'clear_working' => true,
					'hook_extra' => array(
								'plugin' => $plugin
					)
				));

		//Cleanup our hooks, incase something else does a upgrade on this connection.
		remove_filter('upgrader_pre_install', array(&$this, 'deactivate_plugin_before_upgrade'));
		remove_filter('upgrader_clear_destination', array(&$this, 'delete_old_plugin'));

		if ( ! $this->result || is_wp_error($this->result) )
			return $this->result;

		// Force refresh of plugin update information
		delete_transient('update_plugins');
	}

	//return plugin info.
	function plugin_info() {
		if ( ! is_array($this->result) )
			return false;
		if ( empty($this->result['destination_name']) )
			return false;

		$plugin = get_plugins('/' . $this->result['destination_name']); //Ensure to pass with leading slash
		if ( empty($plugin) )
			return false;

		$pluginfiles = array_keys($plugin); //Assume the requested plugin is the first in the list

		return $this->result['destination_name'] . '/' . $pluginfiles[0];
	}

	//Hooked to pre_install
	function deactivate_plugin_before_upgrade($return, $plugin) {

		if ( is_wp_error($return) ) //Bypass.
			return $return;

		$plugin = isset($plugin['plugin']) ? $plugin['plugin'] : '';
		if ( empty($plugin) )
			return new WP_Error('bad_request', $this->strings['bad_request']);

		if ( is_plugin_active($plugin) ) {
			$this->skin->feedback('deactivate_plugin');
			//Deactivate the plugin silently, Prevent deactivation hooks from running.
			deactivate_plugins($plugin, true);
		}
	}

	//Hooked to upgrade_clear_destination
	function delete_old_plugin($removed, $local_destination, $remote_destination, $plugin) {
		global $wp_filesystem;

		if ( is_wp_error($removed) )
			return $removed; //Pass errors through.

		$plugin = isset($plugin['plugin']) ? $plugin['plugin'] : '';
		if ( empty($plugin) )
			return new WP_Error('bad_request', $this->strings['bad_request']);

		$plugins_dir = $wp_filesystem->wp_plugins_dir();
		$this_plugin_dir = trailingslashit( dirname($plugins_dir . $plugin) );

		if ( ! $wp_filesystem->exists($this_plugin_dir) ) //If its already vanished.
			return $removed;

		// If plugin is in its own directory, recursively delete the directory.
		if ( strpos($plugin, '/') && $this_plugin_dir != $plugins_dir ) //base check on if plugin includes directory seperator AND that its not the root plugin folder
			$deleted = $wp_filesystem->delete($this_plugin_dir, true);
		else
			$deleted = $wp_filesystem->delete($plugins_dir . $plugin);

		if ( ! $deleted )
			return new WP_Error('remove_old_failed', $this->strings['remove_old_failed']);

		return $removed;
	}
}

/**
 * Theme Upgrader class for WordPress Themes, It is designed to upgrade/install themes from a local zip, remote zip URL, or uploaded zip file.
 *
 * @TODO More Detailed docs, for methods as well.
 *
 * @package WordPress
 * @subpackage Upgrader
 * @since 2.8.0
 */
class Theme_Upgrader extends WP_Upgrader {

	var $result;

	function upgrade_strings() {
		$this->strings['up_to_date'] = __('The theme is at the latest version.');
		$this->strings['no_package'] = __('Upgrade package not available.');
		$this->strings['downloading_package'] = __('Downloading update from <span class="code">%s</span>.');
		$this->strings['unpack_package'] = __('Unpacking the update.');
		$this->strings['remove_old'] = __('Removing the old version of the theme.');
		$this->strings['remove_old_failed'] = __('Could not remove the old theme.');
		$this->strings['process_failed'] = __('Theme upgrade Failed.');
		$this->strings['process_success'] = __('Theme upgraded successfully.');
	}

	function install_strings() {
		$this->strings['no_package'] = __('Install package not available.');
		$this->strings['downloading_package'] = __('Downloading install package from <span class="code">%s</span>.');
		$this->strings['unpack_package'] = __('Unpacking the package.');
		$this->strings['installing_package'] = __('Installing the theme.');
		$this->strings['process_failed'] = __('Theme Install Failed.');
		$this->strings['process_success'] = __('Theme Installed successfully.');
	}

	function install($package) {

		$this->init();
		$this->install_strings();

		$options = array(
						'package' => $package,
						'destination' => WP_CONTENT_DIR . '/themes',
						'clear_destination' => false, //Do not overwrite files.
						'clear_working' => true
						);

		$this->run($options);

		if ( ! $this->result || is_wp_error($this->result) )
			return $this->result;

		// Force refresh of theme update information
		delete_transient('update_themes');

		if ( empty($result['destination_name']) )
			return false;
		else
			return $result['destination_name'];
	}

	function upgrade($theme) {

		$this->init();
		$this->upgrade_strings();

		// Is an update available?
		$current = get_transient( 'update_themes' );
		if ( !isset( $current->response[ $theme ] ) ) {
			$this->skin->set_result(false);
			$this->skin->error('up_to_date');
			$this->skin->after();
			return false;
		}
		
		$r = $current->response[ $theme ];

		add_filter('upgrader_pre_install', array(&$this, 'current_before'), 10, 2);
		add_filter('upgrader_post_install', array(&$this, 'current_after'), 10, 2);
		add_filter('upgrader_clear_destination', array(&$this, 'delete_old_theme'), 10, 4);

		$options = array(
						'package' => $r['package'],
						'destination' => WP_CONTENT_DIR . '/themes',
						'clear_destination' => true,
						'clear_working' => true,
						'hook_extra' => array(
											'theme' => $theme
											)
						);

		$this->run($options);

		if ( ! $this->result || is_wp_error($this->result) )
			return $this->result;

		// Force refresh of theme update information
		delete_transient('update_themes');

		return true;
	}

	function current_before($return, $theme) {

		if ( is_wp_error($return) )
			return $return;

		$theme = isset($theme['theme']) ? $theme['theme'] : '';

		if ( $theme != get_stylesheet() ) //If not current
			return $return;
		//Change to maintainence mode now.
		$this->maintenance_mode(true);

		return $return;
	}
	function current_after($return, $theme) {
		if ( is_wp_error($return) )
			return $return;

		$theme = isset($theme['theme']) ? $theme['theme'] : '';

		if ( $theme != get_stylesheet() ) //If not current
			return $return;

		//Ensure stylesheet name hasnt changed after the upgrade:
		if ( $theme == get_stylesheet() && $theme != $this->result['destination_name'] ) {
			$theme_info = $this->theme_info();
			$stylesheet = $this->result['destination_name'];
			$template = !empty($theme_info['Template']) ? $theme_info['Template'] : $stylesheet;
			switch_theme($template, $stylesheet, true);
		}

		//Time to remove maintainence mode
		$this->maintenance_mode(false);
		return $return;
	}

	function delete_old_theme($removed, $local_destination, $remote_destination, $theme) {
		global $wp_filesystem;

		$theme = isset($theme['theme']) ? $theme['theme'] : '';

		if ( is_wp_error($removed) || empty($theme) )
			return $removed; //Pass errors through.

		$themes_dir = $wp_filesystem->wp_themes_dir();
		if ( $wp_filesystem->exists( trailingslashit($themes_dir) . $theme ) )
			if ( ! $wp_filesystem->delete( trailingslashit($themes_dir) . $theme, true ) )
				return false;
		return true;
	}

	function theme_info() {
		if ( empty($this->result['destination_name']) )
			return false;
		return get_theme_data(WP_CONTENT_DIR . '/themes/' . $this->result['destination_name'] . '/style.css');
	}

}

/**
 * Core Upgrader class for WordPress. It allows for WordPress to upgrade itself in combiantion with the wp-admin/includes/update-core.php file
 *
 * @TODO More Detailed docs, for methods as well.
 *
 * @package WordPress
 * @subpackage Upgrader
 * @since 2.8.0
 */
class Core_Upgrader extends WP_Upgrader {

	function upgrade_strings() {
		$this->strings['up_to_date'] = __('WordPress is at the latest version.');
		$this->strings['no_package'] = __('Upgrade package not available.');
		$this->strings['downloading_package'] = __('Downloading update from <span class="code">%s</span>.');
		$this->strings['unpack_package'] = __('Unpacking the update.');
		$this->strings['copy_failed'] = __('Could not copy files.');
	}

	function upgrade($current) {
		global $wp_filesystem;

		$this->init();
		$this->upgrade_strings();

		if ( !empty($feedback) )
			add_filter('update_feedback', $feedback);

		// Is an update available?
		if ( !isset( $current->response ) || $current->response == 'latest' )
			return new WP_Error('up_to_date', $this->strings['up_to_date']);

		$res = $this->fs_connect( array(ABSPATH, WP_CONTENT_DIR) );
		if ( is_wp_error($res) )
			return $res;

		$wp_dir = trailingslashit($wp_filesystem->abspath());

		$download = $this->download_package( $current->package );
		if ( is_wp_error($download) )
			return $download;

		$working_dir = $this->unpack_package( $download );
		if ( is_wp_error($working_dir) )
			return $working_dir;

		// Copy update-core.php from the new version into place.
		if ( !$wp_filesystem->copy($working_dir . '/wordpress/wp-admin/includes/update-core.php', $wp_dir . 'wp-admin/includes/update-core.php', true) ) {
			$wp_filesystem->delete($working_dir, true);
			return new WP_Error('copy_failed', $this->strings['copy_failed']);
		}
		$wp_filesystem->chmod($wp_dir . 'wp-admin/includes/update-core.php', FS_CHMOD_FILE);

		require(ABSPATH . 'wp-admin/includes/update-core.php');

		return update_core($working_dir, $wp_dir);
	}

}

/**
 * Generic Skin for the WordPress Upgrader classes. This skin is designed to be extended for specific purposes.
 *
 * @TODO More Detailed docs, for methods as well.
 *
 * @package WordPress
 * @subpackage Upgrader
 * @since 2.8.0
 */
class WP_Upgrader_Skin {

	var $upgrader;
	var $done_header = false;

	function WP_Upgrader_Skin($args = array()) {
		return $this->__construct($args);
	}
	function __construct($args = array()) {
		$defaults = array( 'url' => '', 'nonce' => '', 'title' => '', 'context' => false );
		$this->options = wp_parse_args($args, $defaults);
	}

	function set_upgrader(&$upgrader) {
		if ( is_object($upgrader) )
			$this->upgrader =& $upgrader;
	}
	function set_result($result) {
		$this->result = $result;
	}

	function request_filesystem_credentials($error = false) {
		$url = $this->options['url'];
		$context = $this->options['context'];
		if ( !empty($this->options['nonce']) )
			$url = wp_nonce_url($url, $this->options['nonce']);
		return request_filesystem_credentials($url, '', $error, $context); //Possible to bring inline, Leaving as is for now.
	}

	function header() {
		if ( $this->done_header )
			return;
		$this->done_header = true;
		echo '<div class="wrap">';
		echo screen_icon();
		echo '<h2>' . $this->options['title'] . '</h2>';
	}
	function footer() {
		echo '</div>';
	}

	function error($errors) {
		if ( ! $this->done_header )
			$this->header();
		if ( is_string($errors) ) {
			$this->feedback($errors);
		} elseif ( is_wp_error($errors) && $errors->get_error_code() ) {
			foreach ( $errors->get_error_messages() as $message ) {
				if ( $errors->get_error_data() )
					$this->feedback($message . ' ' . $errors->get_error_data() );
				else
					$this->feedback($message);
			}
		}
	}

	function feedback($string) {
		if ( isset( $this->upgrader->strings[$string] ) )
			$string = $this->upgrader->strings[$string];

		if ( strpos($string, '%') !== false ) {
			$args = func_get_args();
			$args = array_splice($args, 1);
			if ( !empty($args) )
				$string = vsprintf($string, $args);
		}
		if ( empty($string) )
			return;
		show_message($string);
	}
	function before() {}
	function after() {}

}

/**
 * Plugin Upgrader Skin for WordPress Plugin Upgrades.
 *
 * @TODO More Detailed docs, for methods as well.
 *
 * @package WordPress
 * @subpackage Upgrader
 * @since 2.8.0
 */
class Plugin_Upgrader_Skin extends WP_Upgrader_Skin {
	var $plugin = '';
	var $plugin_active = false;

	function Plugin_Upgrader_Skin($args = array()) {
		return $this->__construct($args);
	}

	function __construct($args = array()) {
		$defaults = array( 'url' => '', 'plugin' => '', 'nonce' => '', 'title' => __('Upgrade Plugin') );
		$args = wp_parse_args($args, $defaults);

		$this->plugin = $args['plugin'];

		$this->plugin_active = is_plugin_active($this->plugin);

		parent::__construct($args);
	}

	function after() {
		$this->plugin = $this->upgrader->plugin_info();
		if( !empty($this->plugin) && !is_wp_error($this->result) && $this->plugin_active ){
			show_message(__('Attempting reactivation of the plugin'));
			echo '<iframe style="border:0;overflow:hidden" width="100%" height="170px" src="' . wp_nonce_url('update.php?action=activate-plugin&plugin=' . $this->plugin, 'activate-plugin_' . $this->plugin) .'"></iframe>';
		}
		$update_actions =  array(
			'activate_plugin' => '<a href="' . wp_nonce_url('plugins.php?action=activate&amp;plugin=' . $this->plugin, 'activate-plugin_' . $this->plugin) . '" title="' . esc_attr__('Activate this plugin') . '" target="_parent">' . __('Activate Plugin') . '</a>',
			'plugins_page' => '<a href="' . admin_url('plugins.php') . '" title="' . esc_attr__('Goto plugins page') . '" target="_parent">' . __('Return to Plugins page') . '</a>'
		);
		if ( $this->plugin_active )
			unset( $update_actions['activate_plugin'] );
		if ( ! $this->result || is_wp_error($this->result) )
			unset( $update_actions['activate_plugin'] );

		$update_actions = apply_filters('update_plugin_complete_actions', $update_actions, $this->plugin);
		if ( ! empty($update_actions) )
			$this->feedback('<strong>' . __('Actions:') . '</strong> ' . implode(' | ', (array)$update_actions));
	}
}

/**
 * Plugin Installer Skin for WordPress Plugin Installer.
 *
 * @TODO More Detailed docs, for methods as well.
 *
 * @package WordPress
 * @subpackage Upgrader
 * @since 2.8.0
 */
class Plugin_Installer_Skin extends WP_Upgrader_Skin {
	var $api;
	var $type;

	function Plugin_Installer_Skin($args = array()) {
		return $this->__construct($args);
	}

	function __construct($args = array()) {
		$defaults = array( 'type' => 'web', 'url' => '', 'plugin' => '', 'nonce' => '', 'title' => '' );
		$args = wp_parse_args($args, $defaults);

		$this->type = $args['type'];
		$this->api = isset($args['api']) ? $args['api'] : array();

		parent::__construct($args);
	}

	function before() {
		if ( !empty($this->api) )
			$this->upgrader->strings['process_success'] = sprintf( __('Successfully installed the plugin <strong>%s %s</strong>.'), $this->api->name, $this->api->version);
	}

	function after() {

		$plugin_file = $this->upgrader->plugin_info();

		$install_actions = array(
			'activate_plugin' => '<a href="' . wp_nonce_url('plugins.php?action=activate&amp;plugin=' . $plugin_file, 'activate-plugin_' . $plugin_file) . '" title="' . esc_attr__('Activate this plugin') . '" target="_parent">' . __('Activate Plugin') . '</a>',
							);

		if ( $this->type == 'web' )
			$install_actions['plugins_page'] = '<a href="' . admin_url('plugin-install.php') . '" title="' . esc_attr__('Return to Plugin Installer') . '" target="_parent">' . __('Return to Plugin Installer') . '</a>';
		else
			$install_actions['plugins_page'] = '<a href="' . admin_url('plugins.php') . '" title="' . esc_attr__('Return to Plugins page') . '" target="_parent">' . __('Return to Plugins page') . '</a>';


		if ( ! $this->result || is_wp_error($this->result) )
			unset( $install_actions['activate_plugin'] );

		$install_actions = apply_filters('install_plugin_complete_actions', $install_actions, $this->api, $plugin_file);
		if ( ! empty($install_actions) )
			$this->feedback('<strong>' . __('Actions:') . '</strong> ' . implode(' | ', (array)$install_actions));
	}
}

/**
 * Theme Installer Skin for the WordPress Theme Installer.
 *
 * @TODO More Detailed docs, for methods as well.
 *
 * @package WordPress
 * @subpackage Upgrader
 * @since 2.8.0
 */
class Theme_Installer_Skin extends WP_Upgrader_Skin {
	var $api;
	var $type;

	function Theme_Installer_Skin($args = array()) {
		return $this->__construct($args);
	}

	function __construct($args = array()) {
		$defaults = array( 'type' => 'web', 'url' => '', 'theme' => '', 'nonce' => '', 'title' => '' );
		$args = wp_parse_args($args, $defaults);

		$this->type = $args['type'];
		$this->api = isset($args['api']) ? $args['api'] : array();

		parent::__construct($args);
	}

	function before() {
		if ( !empty($this->api) ) {
			/* translators: 1: theme name, 2: version */
			$this->upgrader->strings['process_success'] = sprintf( __('Successfully installed the theme <strong>%1$s %2$s</strong>.'), $this->api->name, $this->api->version);
		}
	}

	function after() {
		if ( empty($this->upgrader->result['destination_name']) )
			return;

		$theme_info = $this->upgrader->theme_info();
		if ( empty($theme_info) )
			return;
		$name = $theme_info['Name'];
		$stylesheet = $this->upgrader->result['destination_name'];
		$template = !empty($theme_info['Template']) ? $theme_info['Template'] : $stylesheet;

		$preview_link = htmlspecialchars( add_query_arg( array('preview' => 1, 'template' => $template, 'stylesheet' => $stylesheet, 'TB_iframe' => 'true' ), trailingslashit(esc_url(get_option('home'))) ) );
		$activate_link = wp_nonce_url("themes.php?action=activate&amp;template=" . urlencode($template) . "&amp;stylesheet=" . urlencode($stylesheet), 'switch-theme_' . $template);

		$install_actions = array(
			'preview' => '<a href="' . $preview_link . '" class="thickbox thickbox-preview" title="' . esc_attr(sprintf(__('Preview &#8220;%s&#8221;'), $name)) . '">' . __('Preview') . '</a>',
			'activate' => '<a href="' . $activate_link .  '" class="activatelink" title="' . esc_attr( sprintf( __('Activate &#8220;%s&#8221;'), $name ) ) . '">' . __('Activate') . '</a>'
							);

		if ( $this->type == 'web' )
			$install_actions['themes_page'] = '<a href="' . admin_url('theme-install.php') . '" title="' . esc_attr__('Return to Theme Installer') . '" target="_parent">' . __('Return to Theme Installer') . '</a>';
		else
			$install_actions['themes_page'] = '<a href="' . admin_url('themes.php') . '" title="' . esc_attr__('Themes page') . '" target="_parent">' . __('Return to Themes page') . '</a>';

		if ( ! $this->result || is_wp_error($this->result) )
			unset( $install_actions['activate'], $install_actions['preview'] );

		$install_actions = apply_filters('install_theme_complete_actions', $install_actions, $this->api, $stylesheet, $theme_info);
		if ( ! empty($install_actions) )
			$this->feedback('<strong>' . __('Actions:') . '</strong> ' . implode(' | ', (array)$install_actions));
	}
}

/**
 * Theme Upgrader Skin for WordPress Theme Upgrades.
 *
 * @TODO More Detailed docs, for methods as well.
 *
 * @package WordPress
 * @subpackage Upgrader
 * @since 2.8.0
 */
class Theme_Upgrader_Skin extends WP_Upgrader_Skin {
	var $theme = '';

	function Theme_Upgrader_Skin($args = array()) {
		return $this->__construct($args);
	}

	function __construct($args = array()) {
		$defaults = array( 'url' => '', 'theme' => '', 'nonce' => '', 'title' => __('Upgrade Theme') );
		$args = wp_parse_args($args, $defaults);

		$this->theme = $args['theme'];

		parent::__construct($args);
	}

	function after() {

		if ( !empty($this->upgrader->result['destination_name']) &&
			($theme_info = $this->upgrader->theme_info()) &&
			!empty($theme_info) ) {

			$name = $theme_info['Name'];
			$stylesheet = $this->upgrader->result['destination_name'];
			$template = !empty($theme_info['Template']) ? $theme_info['Template'] : $stylesheet;
	
			$preview_link = htmlspecialchars( add_query_arg( array('preview' => 1, 'template' => $template, 'stylesheet' => $stylesheet, 'TB_iframe' => 'true' ), trailingslashit(esc_url(get_option('home'))) ) );
			$activate_link = wp_nonce_url("themes.php?action=activate&amp;template=" . urlencode($template) . "&amp;stylesheet=" . urlencode($stylesheet), 'switch-theme_' . $template);
	
			$update_actions =  array(
				'preview' => '<a href="' . $preview_link . '" class="thickbox thickbox-preview" title="' . esc_attr(sprintf(__('Preview &#8220;%s&#8221;'), $name)) . '">' . __('Preview') . '</a>',
				'activate' => '<a href="' . $activate_link .  '" class="activatelink" title="' . esc_attr( sprintf( __('Activate &#8220;%s&#8221;'), $name ) ) . '">' . __('Activate') . '</a>',
			);
			if ( ( ! $this->result || is_wp_error($this->result) ) || $stylesheet == get_stylesheet() )
				unset($update_actions['preview'], $update_actions['activate']);
		}

		$update_actions['themes_page'] = '<a href="' . admin_url('themes.php') . '" title="' . esc_attr__('Return to Themes page') . '" target="_parent">' . __('Return to Themes page') . '</a>';

		$update_actions = apply_filters('update_theme_complete_actions', $update_actions, $this->theme);
		if ( ! empty($update_actions) )
			$this->feedback('<strong>' . __('Actions:') . '</strong> ' . implode(' | ', (array)$update_actions));
	}
}

/**
 * Upgrade Skin helper for File uploads. This class handles the upload process and passes it as if its a local file to the Upgrade/Installer functions.
 *
 * @TODO More Detailed docs, for methods as well.
 *
 * @package WordPress
 * @subpackage Upgrader
 * @since 2.8.0
 */
class File_Upload_Upgrader {
	var $package;
	var $filename;

	function File_Upload_Upgrader($form, $urlholder) {
		return $this->__construct($form, $urlholder);
	}
	function __construct($form, $urlholder) {
		if ( ! ( ( $uploads = wp_upload_dir() ) && false === $uploads['error'] ) )
			wp_die($uploads['error']);

		if ( empty($_FILES[$form]['name']) && empty($_GET[$urlholder]) )
			wp_die(__('Please select a file'));

		if ( !empty($_FILES) )
			$this->filename = $_FILES[$form]['name'];
		else if ( isset($_GET[$urlholder]) )
			$this->filename = $_GET[$urlholder];

		//Handle a newly uploaded file, Else assume its already been uploaded
		if ( !empty($_FILES) ) {
			$this->filename = wp_unique_filename( $uploads['basedir'], $this->filename );
			$this->package = $uploads['basedir'] . '/' . $this->filename;

			// Move the file to the uploads dir
			if ( false === @ move_uploaded_file( $_FILES[$form]['tmp_name'], $this->package) )
				wp_die( sprintf( __('The uploaded file could not be moved to %s.' ), $uploads['path']));
		} else {
			$this->package = $uploads['basedir'] . '/' . $this->filename;
		}
	}
}
