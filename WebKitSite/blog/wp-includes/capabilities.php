<?php
/**
 * WordPress Roles and Capabilities.
 *
 * @package WordPress
 * @subpackage User
 */

/**
 * WordPress User Roles.
 *
 * The role option is simple, the structure is organized by role name that store
 * the name in value of the 'name' key. The capabilities are stored as an array
 * in the value of the 'capability' key.
 *
 * <code>
 * array (
 *		'rolename' => array (
 *			'name' => 'rolename',
 *			'capabilities' => array()
 *		)
 * )
 * </code>
 *
 * @since 2.0.0
 * @package WordPress
 * @subpackage User
 */
class WP_Roles {
	/**
	 * List of roles and capabilities.
	 *
	 * @since 2.0.0
	 * @access public
	 * @var array
	 */
	var $roles;

	/**
	 * List of the role objects.
	 *
	 * @since 2.0.0
	 * @access public
	 * @var array
	 */
	var $role_objects = array();

	/**
	 * List of role names.
	 *
	 * @since 2.0.0
	 * @access public
	 * @var array
	 */
	var $role_names = array();

	/**
	 * Option name for storing role list.
	 *
	 * @since 2.0.0
	 * @access public
	 * @var string
	 */
	var $role_key;

	/**
	 * Whether to use the database for retrieval and storage.
	 *
	 * @since 2.1.0
	 * @access public
	 * @var bool
	 */
	var $use_db = true;

	/**
	 * PHP4 Constructor - Call {@link WP_Roles::_init()} method.
	 *
	 * @since 2.0.0
	 * @access public
	 *
	 * @return WP_Roles
	 */
	function WP_Roles() {
		$this->_init();
	}

	/**
	 * Setup the object properties.
	 *
	 * The role key is set to the current prefix for the $wpdb object with
	 * 'user_roles' appended. If the $wp_user_roles global is set, then it will
	 * be used and the role option will not be updated or used.
	 *
	 * @since 2.1.0
	 * @access protected
	 * @uses $wpdb Used to get the database prefix.
	 * @global array $wp_user_roles Used to set the 'roles' property value.
	 */
	function _init () {
		global $wpdb;
		global $wp_user_roles;
		$this->role_key = $wpdb->prefix . 'user_roles';
		if ( ! empty( $wp_user_roles ) ) {
			$this->roles = $wp_user_roles;
			$this->use_db = false;
		} else {
			$this->roles = get_option( $this->role_key );
		}

		if ( empty( $this->roles ) )
			return;

		$this->role_objects = array();
		$this->role_names =  array();
		foreach ( (array) $this->roles as $role => $data ) {
			$this->role_objects[$role] = new WP_Role( $role, $this->roles[$role]['capabilities'] );
			$this->role_names[$role] = $this->roles[$role]['name'];
		}
	}

	/**
	 * Add role name with capabilities to list.
	 *
	 * Updates the list of roles, if the role doesn't already exist.
	 *
	 * @since 2.0.0
	 * @access public
	 *
	 * @param string $role Role name.
	 * @param string $display_name Role display name.
	 * @param array $capabilities List of role capabilities.
	 * @return null|WP_Role WP_Role object if role is added, null if already exists.
	 */
	function add_role( $role, $display_name, $capabilities = array() ) {
		if ( isset( $this->roles[$role] ) )
			return;

		$this->roles[$role] = array(
			'name' => $display_name,
			'capabilities' => $capabilities
			);
		if ( $this->use_db )
			update_option( $this->role_key, $this->roles );
		$this->role_objects[$role] = new WP_Role( $role, $capabilities );
		$this->role_names[$role] = $display_name;
		return $this->role_objects[$role];
	}

	/**
	 * Remove role by name.
	 *
	 * @since 2.0.0
	 * @access public
	 *
	 * @param string $role Role name.
	 */
	function remove_role( $role ) {
		if ( ! isset( $this->role_objects[$role] ) )
			return;

		unset( $this->role_objects[$role] );
		unset( $this->role_names[$role] );
		unset( $this->roles[$role] );

		if ( $this->use_db )
			update_option( $this->role_key, $this->roles );
	}

	/**
	 * Add capability to role.
	 *
	 * @since 2.0.0
	 * @access public
	 *
	 * @param string $role Role name.
	 * @param string $cap Capability name.
	 * @param bool $grant Optional, default is true. Whether role is capable of preforming capability.
	 */
	function add_cap( $role, $cap, $grant = true ) {
		$this->roles[$role]['capabilities'][$cap] = $grant;
		if ( $this->use_db )
			update_option( $this->role_key, $this->roles );
	}

	/**
	 * Remove capability from role.
	 *
	 * @since 2.0.0
	 * @access public
	 *
	 * @param string $role Role name.
	 * @param string $cap Capability name.
	 */
	function remove_cap( $role, $cap ) {
		unset( $this->roles[$role]['capabilities'][$cap] );
		if ( $this->use_db )
			update_option( $this->role_key, $this->roles );
	}

	/**
	 * Retrieve role object by name.
	 *
	 * @since 2.0.0
	 * @access public
	 *
	 * @param string $role Role name.
	 * @return object|null Null, if role does not exist. WP_Role object, if found.
	 */
	function &get_role( $role ) {
		if ( isset( $this->role_objects[$role] ) )
			return $this->role_objects[$role];
		else
			return null;
	}

	/**
	 * Retrieve list of role names.
	 *
	 * @since 2.0.0
	 * @access public
	 *
	 * @return array List of role names.
	 */
	function get_names() {
		return $this->role_names;
	}

	/**
	 * Whether role name is currently in the list of available roles.
	 *
	 * @since 2.0.0
	 * @access public
	 *
	 * @param string $role Role name to look up.
	 * @return bool
	 */
	function is_role( $role )
	{
		return isset( $this->role_names[$role] );
	}
}

/**
 * WordPress Role class.
 *
 * @since 2.0.0
 * @package WordPress
 * @subpackage User
 */
class WP_Role {
	/**
	 * Role name.
	 *
	 * @since 2.0.0
	 * @access public
	 * @var string
	 */
	var $name;

	/**
	 * List of capabilities the role contains.
	 *
	 * @since 2.0.0
	 * @access public
	 * @var array
	 */
	var $capabilities;

	/**
	 * PHP4 Constructor - Setup object properties.
	 *
	 * The list of capabilities, must have the key as the name of the capability
	 * and the value a boolean of whether it is granted to the role or not.
	 *
	 * @since 2.0.0
	 * @access public
	 *
	 * @param string $role Role name.
	 * @param array $capabilities List of capabilities.
	 * @return WP_Role
	 */
	function WP_Role( $role, $capabilities ) {
		$this->name = $role;
		$this->capabilities = $capabilities;
	}

	/**
	 * Assign role a capability.
	 *
	 * @see WP_Roles::add_cap() Method uses implementation for role.
	 * @since 2.0.0
	 * @access public
	 *
	 * @param string $cap Capability name.
	 * @param bool $grant Whether role has capability privilege.
	 */
	function add_cap( $cap, $grant = true ) {
		global $wp_roles;

		if ( ! isset( $wp_roles ) )
			$wp_roles = new WP_Roles();

		$this->capabilities[$cap] = $grant;
		$wp_roles->add_cap( $this->name, $cap, $grant );
	}

	/**
	 * Remove capability from role.
	 *
	 * This is a container for {@link WP_Roles::remove_cap()} to remove the
	 * capability from the role. That is to say, that {@link
	 * WP_Roles::remove_cap()} implements the functionality, but it also makes
	 * sense to use this class, because you don't need to enter the role name.
	 *
	 * @since 2.0.0
	 * @access public
	 *
	 * @param string $cap Capability name.
	 */
	function remove_cap( $cap ) {
		global $wp_roles;

		if ( ! isset( $wp_roles ) )
			$wp_roles = new WP_Roles();

		unset( $this->capabilities[$cap] );
		$wp_roles->remove_cap( $this->name, $cap );
	}

	/**
	 * Whether role has capability.
	 *
	 * The capabilities is passed through the 'role_has_cap' filter. The first
	 * parameter for the hook is the list of capabilities the class has
	 * assigned. The second parameter is the capability name to look for. The
	 * third and final parameter for the hook is the role name.
	 *
	 * @since 2.0.0
	 * @access public
	 *
	 * @param string $cap Capability name.
	 * @return bool True, if user has capability. False, if doesn't have capability.
	 */
	function has_cap( $cap ) {
		$capabilities = apply_filters( 'role_has_cap', $this->capabilities, $cap, $this->name );
		if ( !empty( $capabilities[$cap] ) )
			return $capabilities[$cap];
		else
			return false;
	}

}

/**
 * WordPress User class.
 *
 * @since 2.0.0
 * @package WordPress
 * @subpackage User
 */
class WP_User {
	/**
	 * User data container.
	 *
	 * This will be set as properties of the object.
	 *
	 * @since 2.0.0
	 * @access private
	 * @var array
	 */
	var $data;

	/**
	 * The user's ID.
	 *
	 * @since 2.1.0
	 * @access public
	 * @var int
	 */
	var $ID = 0;

	/**
	 * The deprecated user's ID.
	 *
	 * @since 2.0.0
	 * @access public
	 * @deprecated Use WP_User::$ID
	 * @see WP_User::$ID
	 * @var int
	 */
	var $id = 0;

	/**
	 * The individual capabilities the user has been given.
	 *
	 * @since 2.0.0
	 * @access public
	 * @var array
	 */
	var $caps = array();

	/**
	 * User metadata option name.
	 *
	 * @since 2.0.0
	 * @access public
	 * @var string
	 */
	var $cap_key;

	/**
	 * The roles the user is part of.
	 *
	 * @since 2.0.0
	 * @access public
	 * @var array
	 */
	var $roles = array();

	/**
	 * All capabilities the user has, including individual and role based.
	 *
	 * @since 2.0.0
	 * @access public
	 * @var array
	 */
	var $allcaps = array();

	/**
	 * First name of the user.
	 *
	 * Created to prevent notices.
	 *
	 * @since 2.7.0
	 * @access public
	 * @var string
	 */
	var $first_name = '';

	/**
	 * Last name of the user.
	 *
	 * Created to prevent notices.
	 *
	 * @since 2.7.0
	 * @access public
	 * @var string
	 */
	var $last_name = '';

	/**
	 * PHP4 Constructor - Sets up the object properties.
	 *
	 * Retrieves the userdata and then assigns all of the data keys to direct
	 * properties of the object. Calls {@link WP_User::_init_caps()} after
	 * setting up the object's user data properties.
	 *
	 * @since 2.0.0
	 * @access public
	 *
	 * @param int|string $id User's ID or username
	 * @param int $name Optional. User's username
	 * @return WP_User
	 */
	function WP_User( $id, $name = '' ) {

		if ( empty( $id ) && empty( $name ) )
			return;

		if ( ! is_numeric( $id ) ) {
			$name = $id;
			$id = 0;
		}

		if ( ! empty( $id ) )
			$this->data = get_userdata( $id );
		else
			$this->data = get_userdatabylogin( $name );

		if ( empty( $this->data->ID ) )
			return;

		foreach ( get_object_vars( $this->data ) as $key => $value ) {
			$this->{$key} = $value;
		}

		$this->id = $this->ID;
		$this->_init_caps();
	}

	/**
	 * Setup capability object properties.
	 *
	 * Will set the value for the 'cap_key' property to current database table
	 * prefix, followed by 'capabilities'. Will then check to see if the
	 * property matching the 'cap_key' exists and is an array. If so, it will be
	 * used.
	 *
	 * @since 2.1.0
	 * @access protected
	 */
	function _init_caps() {
		global $wpdb;
		$this->cap_key = $wpdb->prefix . 'capabilities';
		$this->caps = &$this->{$this->cap_key};
		if ( ! is_array( $this->caps ) )
			$this->caps = array();
		$this->get_role_caps();
	}

	/**
	 * Retrieve all of the role capabilities and merge with individual capabilities.
	 *
	 * All of the capabilities of the roles the user belongs to are merged with
	 * the users individual roles. This also means that the user can be denied
	 * specific roles that their role might have, but the specific user isn't
	 * granted permission to.
	 *
	 * @since 2.0.0
	 * @uses $wp_roles
	 * @access public
	 */
	function get_role_caps() {
		global $wp_roles;

		if ( ! isset( $wp_roles ) )
			$wp_roles = new WP_Roles();

		//Filter out caps that are not role names and assign to $this->roles
		if ( is_array( $this->caps ) )
			$this->roles = array_filter( array_keys( $this->caps ), array( &$wp_roles, 'is_role' ) );

		//Build $allcaps from role caps, overlay user's $caps
		$this->allcaps = array();
		foreach ( (array) $this->roles as $role ) {
			$role =& $wp_roles->get_role( $role );
			$this->allcaps = array_merge( (array) $this->allcaps, (array) $role->capabilities );
		}
		$this->allcaps = array_merge( (array) $this->allcaps, (array) $this->caps );
	}

	/**
	 * Add role to user.
	 *
	 * Updates the user's meta data option with capabilities and roles.
	 *
	 * @since 2.0.0
	 * @access public
	 *
	 * @param string $role Role name.
	 */
	function add_role( $role ) {
		$this->caps[$role] = true;
		update_usermeta( $this->ID, $this->cap_key, $this->caps );
		$this->get_role_caps();
		$this->update_user_level_from_caps();
	}

	/**
	 * Remove role from user.
	 *
	 * @since 2.0.0
	 * @access public
	 *
	 * @param string $role Role name.
	 */
	function remove_role( $role ) {
		if ( empty( $this->roles[$role] ) || ( count( $this->roles ) <= 1 ) )
			return;
		unset( $this->caps[$role] );
		update_usermeta( $this->ID, $this->cap_key, $this->caps );
		$this->get_role_caps();
	}

	/**
	 * Set the role of the user.
	 *
	 * This will remove the previous roles of the user and assign the user the
	 * new one. You can set the role to an empty string and it will remove all
	 * of the roles from the user.
	 *
	 * @since 2.0.0
	 * @access public
	 *
	 * @param string $role Role name.
	 */
	function set_role( $role ) {
		foreach ( (array) $this->roles as $oldrole )
			unset( $this->caps[$oldrole] );
		if ( !empty( $role ) ) {
			$this->caps[$role] = true;
			$this->roles = array( $role => true );
		} else {
			$this->roles = false;
		}
		update_usermeta( $this->ID, $this->cap_key, $this->caps );
		$this->get_role_caps();
		$this->update_user_level_from_caps();
	}

	/**
	 * Choose the maximum level the user has.
	 *
	 * Will compare the level from the $item parameter against the $max
	 * parameter. If the item is incorrect, then just the $max parameter value
	 * will be returned.
	 *
	 * Used to get the max level based on the capabilities the user has. This
	 * is also based on roles, so if the user is assigned the Administrator role
	 * then the capability 'level_10' will exist and the user will get that
	 * value.
	 *
	 * @since 2.0.0
	 * @access public
	 *
	 * @param int $max Max level of user.
	 * @param string $item Level capability name.
	 * @return int Max Level.
	 */
	function level_reduction( $max, $item ) {
		if ( preg_match( '/^level_(10|[0-9])$/i', $item, $matches ) ) {
			$level = intval( $matches[1] );
			return max( $max, $level );
		} else {
			return $max;
		}
	}

	/**
	 * Update the maximum user level for the user.
	 *
	 * Updates the 'user_level' user metadata (includes prefix that is the
	 * database table prefix) with the maximum user level. Gets the value from
	 * the all of the capabilities that the user has.
	 *
	 * @since 2.0.0
	 * @access public
	 */
	function update_user_level_from_caps() {
		global $wpdb;
		$this->user_level = array_reduce( array_keys( $this->allcaps ), array( &$this, 'level_reduction' ), 0 );
		update_usermeta( $this->ID, $wpdb->prefix.'user_level', $this->user_level );
	}

	/**
	 * Add capability and grant or deny access to capability.
	 *
	 * @since 2.0.0
	 * @access public
	 *
	 * @param string $cap Capability name.
	 * @param bool $grant Whether to grant capability to user.
	 */
	function add_cap( $cap, $grant = true ) {
		$this->caps[$cap] = $grant;
		update_usermeta( $this->ID, $this->cap_key, $this->caps );
	}

	/**
	 * Remove capability from user.
	 *
	 * @since 2.0.0
	 * @access public
	 *
	 * @param string $cap Capability name.
	 */
	function remove_cap( $cap ) {
		if ( empty( $this->caps[$cap] ) ) return;
		unset( $this->caps[$cap] );
		update_usermeta( $this->ID, $this->cap_key, $this->caps );
	}

	/**
	 * Remove all of the capabilities of the user.
	 *
	 * @since 2.1.0
	 * @access public
	 */
	function remove_all_caps() {
		global $wpdb;
		$this->caps = array();
		update_usermeta( $this->ID, $this->cap_key, '' );
		update_usermeta( $this->ID, $wpdb->prefix.'user_level', '' );
		$this->get_role_caps();
	}

	/**
	 * Whether user has capability or role name.
	 *
	 * This is useful for looking up whether the user has a specific role
	 * assigned to the user. The second optional parameter can also be used to
	 * check for capabilities against a specfic post.
	 *
	 * @since 2.0.0
	 * @access public
	 *
	 * @param string|int $cap Capability or role name to search.
	 * @param int $post_id Optional. Post ID to check capability against specific post.
	 * @return bool True, if user has capability; false, if user does not have capability.
	 */
	function has_cap( $cap ) {
		if ( is_numeric( $cap ) )
			$cap = $this->translate_level_to_cap( $cap );

		$args = array_slice( func_get_args(), 1 );
		$args = array_merge( array( $cap, $this->ID ), $args );
		$caps = call_user_func_array( 'map_meta_cap', $args );
		// Must have ALL requested caps
		$capabilities = apply_filters( 'user_has_cap', $this->allcaps, $caps, $args );
		foreach ( (array) $caps as $cap ) {
			//echo "Checking cap $cap<br />";
			if ( empty( $capabilities[$cap] ) || !$capabilities[$cap] )
				return false;
		}

		return true;
	}

	/**
	 * Convert numeric level to level capability name.
	 *
	 * Prepends 'level_' to level number.
	 *
	 * @since 2.0.0
	 * @access public
	 *
	 * @param int $level Level number, 1 to 10.
	 * @return string
	 */
	function translate_level_to_cap( $level ) {
		return 'level_' . $level;
	}

}

/**
 * Map meta capabilities to primitive capabilities.
 *
 * This does not actually compare whether the user ID has the actual capability,
 * just what the capability or capabilities are. Meta capability list value can
 * be 'delete_user', 'edit_user', 'delete_post', 'delete_page', 'edit_post',
 * 'edit_page', 'read_post', or 'read_page'.
 *
 * @since 2.0.0
 *
 * @param string $cap Capability name.
 * @param int $user_id User ID.
 * @return array Actual capabilities for meta capability.
 */
function map_meta_cap( $cap, $user_id ) {
	$args = array_slice( func_get_args(), 2 );
	$caps = array();

	switch ( $cap ) {
	case 'delete_user':
		$caps[] = 'delete_users';
		break;
	case 'edit_user':
		if ( !isset( $args[0] ) || $user_id != $args[0] ) {
			$caps[] = 'edit_users';
		}
		break;
	case 'delete_post':
		$author_data = get_userdata( $user_id );
		//echo "post ID: {$args[0]}<br />";
		$post = get_post( $args[0] );
		if ( 'page' == $post->post_type ) {
			$args = array_merge( array( 'delete_page', $user_id ), $args );
			return call_user_func_array( 'map_meta_cap', $args );
		}
		$post_author_data = get_userdata( $post->post_author );
		//echo "current user id : $user_id, post author id: " . $post_author_data->ID . "<br />";
		// If the user is the author...
		if ( $user_id == $post_author_data->ID ) {
			// If the post is published...
			if ( 'publish' == $post->post_status )
				$caps[] = 'delete_published_posts';
			else
				// If the post is draft...
				$caps[] = 'delete_posts';
		} else {
			// The user is trying to edit someone else's post.
			$caps[] = 'delete_others_posts';
			// The post is published, extra cap required.
			if ( 'publish' == $post->post_status )
				$caps[] = 'delete_published_posts';
			elseif ( 'private' == $post->post_status )
				$caps[] = 'delete_private_posts';
		}
		break;
	case 'delete_page':
		$author_data = get_userdata( $user_id );
		//echo "post ID: {$args[0]}<br />";
		$page = get_page( $args[0] );
		$page_author_data = get_userdata( $page->post_author );
		//echo "current user id : $user_id, page author id: " . $page_author_data->ID . "<br />";
		// If the user is the author...
		if ( $user_id == $page_author_data->ID ) {
			// If the page is published...
			if ( $page->post_status == 'publish' )
				$caps[] = 'delete_published_pages';
			else
				// If the page is draft...
				$caps[] = 'delete_pages';
		} else {
			// The user is trying to edit someone else's page.
			$caps[] = 'delete_others_pages';
			// The page is published, extra cap required.
			if ( $page->post_status == 'publish' )
				$caps[] = 'delete_published_pages';
			elseif ( $page->post_status == 'private' )
				$caps[] = 'delete_private_pages';
		}
		break;
		// edit_post breaks down to edit_posts, edit_published_posts, or
		// edit_others_posts
	case 'edit_post':
		$author_data = get_userdata( $user_id );
		//echo "post ID: {$args[0]}<br />";
		$post = get_post( $args[0] );
		if ( 'page' == $post->post_type ) {
			$args = array_merge( array( 'edit_page', $user_id ), $args );
			return call_user_func_array( 'map_meta_cap', $args );
		}
		$post_author_data = get_userdata( $post->post_author );
		//echo "current user id : $user_id, post author id: " . $post_author_data->ID . "<br />";
		// If the user is the author...
		if ( $user_id == $post_author_data->ID ) {
			// If the post is published...
			if ( 'publish' == $post->post_status )
				$caps[] = 'edit_published_posts';
			else
				// If the post is draft...
				$caps[] = 'edit_posts';
		} else {
			// The user is trying to edit someone else's post.
			$caps[] = 'edit_others_posts';
			// The post is published, extra cap required.
			if ( 'publish' == $post->post_status )
				$caps[] = 'edit_published_posts';
			elseif ( 'private' == $post->post_status )
				$caps[] = 'edit_private_posts';
		}
		break;
	case 'edit_page':
		$author_data = get_userdata( $user_id );
		//echo "post ID: {$args[0]}<br />";
		$page = get_page( $args[0] );
		$page_author_data = get_userdata( $page->post_author );
		//echo "current user id : $user_id, page author id: " . $page_author_data->ID . "<br />";
		// If the user is the author...
		if ( $user_id == $page_author_data->ID ) {
			// If the page is published...
			if ( 'publish' == $page->post_status )
				$caps[] = 'edit_published_pages';
			else
				// If the page is draft...
				$caps[] = 'edit_pages';
		} else {
			// The user is trying to edit someone else's page.
			$caps[] = 'edit_others_pages';
			// The page is published, extra cap required.
			if ( 'publish' == $page->post_status )
				$caps[] = 'edit_published_pages';
			elseif ( 'private' == $page->post_status )
				$caps[] = 'edit_private_pages';
		}
		break;
	case 'read_post':
		$post = get_post( $args[0] );
		if ( 'page' == $post->post_type ) {
			$args = array_merge( array( 'read_page', $user_id ), $args );
			return call_user_func_array( 'map_meta_cap', $args );
		}

		if ( 'private' != $post->post_status ) {
			$caps[] = 'read';
			break;
		}

		$author_data = get_userdata( $user_id );
		$post_author_data = get_userdata( $post->post_author );
		if ( $user_id == $post_author_data->ID )
			$caps[] = 'read';
		else
			$caps[] = 'read_private_posts';
		break;
	case 'read_page':
		$page = get_page( $args[0] );

		if ( 'private' != $page->post_status ) {
			$caps[] = 'read';
			break;
		}

		$author_data = get_userdata( $user_id );
		$page_author_data = get_userdata( $page->post_author );
		if ( $user_id == $page_author_data->ID )
			$caps[] = 'read';
		else
			$caps[] = 'read_private_pages';
		break;
	case 'unfiltered_upload':
		if ( defined('ALLOW_UNFILTERED_UPLOADS') && ALLOW_UNFILTERED_UPLOADS == true )
			$caps[] = $cap;
		else
			$caps[] = 'do_not_allow';
		break;
	default:
		// If no meta caps match, return the original cap.
		$caps[] = $cap;
	}

	return apply_filters('map_meta_cap', $caps, $cap, $user_id, $args);
}

/**
 * Whether current user has capability or role.
 *
 * @since 2.0.0
 *
 * @param string $capability Capability or role name.
 * @return bool
 */
function current_user_can( $capability ) {
	$current_user = wp_get_current_user();

	if ( empty( $current_user ) )
		return false;

	$args = array_slice( func_get_args(), 1 );
	$args = array_merge( array( $capability ), $args );

	return call_user_func_array( array( &$current_user, 'has_cap' ), $args );
}

/**
 * Retrieve role object.
 *
 * @see WP_Roles::get_role() Uses method to retrieve role object.
 * @since 2.0.0
 *
 * @param string $role Role name.
 * @return object
 */
function get_role( $role ) {
	global $wp_roles;

	if ( ! isset( $wp_roles ) )
		$wp_roles = new WP_Roles();

	return $wp_roles->get_role( $role );
}

/**
 * Add role, if it does not exist.
 *
 * @see WP_Roles::add_role() Uses method to add role.
 * @since 2.0.0
 *
 * @param string $role Role name.
 * @param string $display_name Display name for role.
 * @param array $capabilities List of capabilities.
 * @return null|WP_Role WP_Role object if role is added, null if already exists.
 */
function add_role( $role, $display_name, $capabilities = array() ) {
	global $wp_roles;

	if ( ! isset( $wp_roles ) )
		$wp_roles = new WP_Roles();

	return $wp_roles->add_role( $role, $display_name, $capabilities );
}

/**
 * Remove role, if it exists.
 *
 * @see WP_Roles::remove_role() Uses method to remove role.
 * @since 2.0.0
 *
 * @param string $role Role name.
 * @return null
 */
function remove_role( $role ) {
	global $wp_roles;

	if ( ! isset( $wp_roles ) )
		$wp_roles = new WP_Roles();

	return $wp_roles->remove_role( $role );
}

?>
