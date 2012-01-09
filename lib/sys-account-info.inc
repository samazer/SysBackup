<?php // -*- mode: php; coding: utf-8-unix; -*- 
/**
 *
 * Account Information (Users & Groups)
 *
 * <p><b>Copyright</b>
 *
 * <p>Copyright (c) 2001-2011 by Sam Azer, All Rights Reserved.
 *
 * <p>This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * <p>This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * <p>You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 * @author Sam Azer <sam@azertech.net>
 * @version 1.0
 * @package aforms
 * @subpackage lib-sys
 * @license http://www.gnu.org/licenses/agpl.html AGPL
 * @copyright 2009-2011 by Sam Azer, All rights reserved
 * @link http://www.azertech.net/
 * @link http://www.samazer.net/about
 *
 *
 */


/**#@+
 * Global Defined Constant
 * System User and Group account number ranges: min/max uid for system accounts, user accounts and system accts with high UIDs
 */
define( "SYSTEM_MINSYS1UID", 0 );
define( "SYSTEM_MAXSYS1UID", 499 );
/**#@-*/

/**#@+
 * Global Defined Constant
 * System User and Group account number ranges: user accounts (change the min for Debian/Ubuntu to 1000)
 */
define( "SYSTEM_MINUSRUID", 500 );
define( "SYSTEM_MAXUSRUID", 19999 );
/**#@-*/

/**#@+
 * Global Defined Constant
 * System User and Group account number ranges: for accounts like nfsnobody with a UID of 65534
 */
define( "SYSTEM_MINSYS2UID", 32000 );
define( "SYSTEM_MAXSYS2UID", 65535 );
/**#@-*/

/**#@+
 * Global Defined Constant
 * System User and Group account number ranges: min/max gid for system accounts
 */
define( "SYSTEM_MINSYSGID", 0 );
define( "SYSTEM_MAXSYSGID", 499 );
/**#@-*/

/**#@+
 * Global Defined Constant
 * System User and Group account number ranges: min/max gid for user accounts (change the min for Debian/Ubuntu to 1000)
 */
define( "SYSTEM_MINUSRGID", 500 );
define( "SYSTEM_MAXUSRGID", 19999 );
/**#@-*/

/**
 * Account Information Class
 *
 * The Account Information class is primarily used to collect
 * information from the system password & group files. The information
 * is loaded into the properties called users and groups. Additional
 * methods are available to export the information in the form of
 * standard Linux administration commands. When executed the commands
 * will reconstruct the original user and group information.
 *
 * @package aforms
 * @subpackage lib-sys
 *
 */
class account_info_c
{
  /**
   *
   * The directory to scan. Defaults to /etc/. Should contain the
   * path to copies of /etc/passwd, /etc/group, /etc/shadow,
   * etc. files from the server being upgraded.
   *
   * @var string $dir 
   *
   */
   var $dir;

  /**
   *
   * The hostname of the server as returned by the `hostname` command.
   *
   * @var string $host 
   *
   */
   var $host;

  /**
   *
   * An array containing the information associated with each user.
   *
   * <p>The primary index into the array is the userid. 
   * <p>The second index into the array is one of:
   * <ul>
   * <li>uid    - numeric uid for the user
   * <li>gid    - numeric base gid for the user
   * <li>ident  - the GECOS field contents
   * <li>dir    - the users home dir
   * <li>shell  - the login shell for the user
   * <li>groups - the list of groups that this user is a member of
   * <li>passwd - the users password (encrypted)
   * <li>lstchg - days since Jan 1, 1970 password was last changed
   * <li>min    - days before which password may not be changed
   * <li>max    - days after which password must be changed
   * <li>warn   - days before password is to expire that user is warned of pending password expiration
   * <li>inact  - days after password expires that account is considered inactive and disabled
   * <li>expire - days since Jan 1, 1970 when account will be disabled
   * <li>flag   - (reserved for future use)
   * </ul>
   * 
   * <p>For example: to get the home directory for user fred:
   * <code>$homedir = $this->users["fred"]["dir"];</code>
   *
   * @var array $users
   *
   */
   var $users;

  /**
   *
   * An array containing the information associated with each system group.
   *
   * <p>The primary index into the array is the group id.
   * <p>The second index into the array is one of:
   * <ul>
   * <li>gid     - numeric base gid for this group
   * <li>passwd  - encrypted password for this group
   * <li>admins  - comma-separated list of group administrators
   * <li>members - comma-separated list of group members
   * </ul>
   * 
   * <p>For example: to get the list of members of the group webmasters:
   * <code>$member_list = $this->groups["webmasters"]["members"];</code>
   *
   * @var array $groups
   *
   */
   var $groups;


  /**
   *
   * Get user info
   *
   * <p>Simple (pro forma) function to return information about a
   * single user. This function is not currently being used but might
   * some day be used by future scripts.
   *
   * <p>The first parameter (required) must contain the user id, ie:
   * "fred" 
   *
   * <p>The second parameter must contain the item name. This
   * is the secondary index to the $users property as described
   * elsewhere.
   *
   * <p>For example: to get the home directory for user fred:
   * <code>$homedir = $account_info_obj->user( "fred", "dir" );</code>
   *
   * @param string $userid the id string for the user being queried. 
   *    This parameter is required.
   *
   * @see $users
   *
   * @param string $item the secondary index to the users
   *    property as described elsewhere. This parameter is
   *    optional. If the parameter is not passed the method
   *    will return an array containing all available information for
   *    the specified user.
   *
   * @return string|array|false $info returns the requested item or
   *    all available items if no item is specified. Returns false if
   *    the $userid or $item name is not known.
   *
   */
   function user( $userid, $item = false )
   {
      if ( !isset($this->users[$userid]) )
         return false;

      if ( $item === false )
         return $this->users[$userid];

      if ( !isset( $this->users[$userid][$item] ) )
         return false;

      return $this->users[$userid][$item];
   }


  /**
   *
   * Get group info
   *
   * <p>Simple (pro forma) function to return information about a
   * group. This function is not currently being used but might
   * some day be used by future scripts.
   *
   * <p>The first parameter (required) must contain the group id, ie:
   * "webmasters" 
   *
   * <p>The second parameter must contain the item name. This
   * is the secondary index to the $groups property as described
   * elsewhere.
   *
   * <p>For example: to get the list of members of the webmasters group:
   * <code>$member_list = $account_info_obj->group( "webmasters", "members" );</code>
   *
   * @param string $groupid the id string for the group being queried. 
   *    This parameter is required.
   *
   * @see $groups
   *
   * @param string $item the secondary index to the groups
   *    property as described elsewhere. This parameter is
   *    optional. If the parameter is not passed the method
   *    will return an array containing all available information for
   *    the specified group.
   *
   * @return string|array|false $info returns the requested item or
   *    all available items if no item is specified. Returns false if
   *    the $groupid or $item name is not known.
   *
   */
   function group( $groupid, $item = false )
   {
      if ( !isset($this->groups[$groupid]) )
         return false;

      if ( $item === false )
         return $this->groups[$groupid];

      if ( !isset( $this->groups[$groupid][$item] ) )
         return false;

      return $this->groups[$groupid][$item];
   }


  /**
   *
   * Read the Password File
   *
   * <p>initialization function to scan the system password file. The $users property is populated with the information found.
   *
   * @param string $dir the path to the directory containing the passwd file.
   *    This parameter is required.
   *
   * @see __construct()
   * @see $users
   *
   * @return void
   *
   */
   function read_passwd( $dir )
   {
      $passwd_file = explode( "\n", file_get_contents( $dir . "passwd" ) );
      foreach ( $passwd_file as $user_info )
         if ( $user_info > "" )
         {
            $info = explode( ":", $user_info );
            $this->users[$info[0]] = array( "uid"   => (int)$info[2],
                                            "gid"   => (int)$info[3],
                                            "ident" => addslashes($info[4]), // GECOS field
                                            "dir"   => $info[5],
                                            "shell" => $info[6],
                                            "groups" => "",
                                         );
         }
   }


  /**
   *
   * Read the Shadow File
   *
   * <p>initialization function to scan the system shadow file. The
   * $users property is populated with the information found. (Note
   * that this class assumes that your distro is using shadow files.)
   *
   * @param string $dir the path to the directory containing the passwd file.
   *    This parameter is required.
   *
   * @see __construct()
   * @see $users
   *
   * @return void
   *
   */
   function read_shadow( $dir )
   {
      $passwd_file = explode( "\n", file_get_contents( $dir . "shadow" ) );
      foreach ( $passwd_file as $user_info )
         if ( $user_info > "" )
         {
            $info = explode( ":", $user_info );
            if ( $info[1] == "!!" )
               $info[1] = ""; // don't set password for accounts that don't have one
            else $info[1] = str_replace( "$", "\\$", addslashes( $info[1] ) );

            $this->users[$info[0]] = $this->users[$info[0]]
                                   + array( "passwd" => $info[1], // encrypted password
                                            "lstchg" => $info[2], // days since Jan 1, 1970 password was last changed
                                            "min"    => $info[3], // days before which password may not be changed
                                            "max"    => $info[4], // days after which password must be changed
                                            "warn"   => $info[5], // days before password is to expire that user is warned of pending password expiration
                                            "inact"  => $info[6], // days after password expires that account is considered inactive and disabled
                                            "expire" => $info[7], // days since Jan 1, 1970 when account will be disabled
                                            "flag"   => $info[8], // reserved for future use
                                         );
         }
   }


  /**
   *
   * Read the Group File
   *
   * <p>initialization function to scan the system group file. The
   * $groups property is populated with the information found. 
   *
   * @param string $dir the path to the directory containing the passwd file.
   *    This parameter is required.
   *
   * @see __construct()
   * @see $groups
   *
   * @return void
   *
   */
   function read_group( $dir )
   {
      $group_file = explode( "\n", file_get_contents( $dir . "group" ) );
      foreach ( $group_file as $group_info )
         if ( $group_info > "" )
         {
            $info = explode( ":", $group_info );
            $this->groups[$info[0]] = array( "gid"     => (int)$info[2],
                                             "members" => $info[3],
                                         );
         }
   }


  /**
   *
   * Read the Group Shadow File
   *
   * <p>initialization function to scan the system group shadow
   * file. The $groups property is populated with the information
   * found. The $users property is also updated with group membership
   * information.  (Note that that this class assumes that your
   * distro is using shadow files.)
   *
   * @param string $dir the path to the directory containing the passwd file.
   *    This parameter is required.
   *
   * @see __construct()
   * @see $groups
   * @see $users
   *
   * @return void
   *
   */
   function read_gshadow( $dir )
   {
      $group_file = explode( "\n", file_get_contents( $dir . "gshadow" ) );
      foreach ( $group_file as $group_info )
         if ( $group_info > "" )
         {
            $info = explode( ":", $group_info );
            $this->groups[$info[0]] = $this->groups[$info[0]]
                                    + array( "passwd" => $info[1],  // encrypted password
                                             "admins" => $info[2],  // comma-separated list of group administrators
                                             "members" => $info[3], // comma-separated list of group members
                                         );

            if ( $info[3] > "" )
            {
               $group = $info[0];
               $members = explode( ",", str_replace(" ", "", $info[3]) );
               foreach ( $members as $u )
                  if ( $u > "" )
                     $this->users[$u]["groups"] = $this->users[$u]["groups"]
                                                . ($this->users[$u]["groups"] > "" ? "," : "")
                                                . $group
                                                ;
            }
         }
   }


  /**
   *
   * Object constructor
   *
   * <p>initialization function that calls the various initialization
   * methods. The single parameter, $dir, is saved into the $dir
   * property of the object. The $host property is populated with the
   * output from the hostname command. This is used only for
   * informational purposes when the data is being exported.  
   *
   * <p>Once again it is important to note that this class assumes
   * your distro is using shadow files. If not you should comment-out
   * the calls to the read_shadow() and read_gshadow() methods.
   *
   * @param string $dir the path to the directory containing the passwd file.
   *    This parameter is required.
   *
   * @see $groups
   * @see $users
   * @see $dir
   * @see $host
   * @see read_passwd()
   * @see read_group()
   * @see read_shadow()
   * @see read_gshadow()
   *
   * @return void
   *
   */
   function __construct( $dir )
   {
      $this->dir = $dir;
      $this->host = substr(`hostname`, 0, -1);

      $this->read_passwd( $dir );
      $this->read_group( $dir );
      $this->read_shadow( $dir );
      $this->read_gshadow( $dir );
   }
}

