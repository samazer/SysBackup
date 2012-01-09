<?php // -*- mode: php; mode: mmm; coding: utf-8-unix; -*-
/**
 *
 * Account Migration (Users & Groups)
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


/**
 * Account Info Class
 *
 * The Account Info class scans the system files to collect account
 * information.
 *
 */
include "lib/sys-account-info.inc";


/**
 * Account Migration Class
 *
 * The Account Migration class is used to export system account
 * information in the form of standard Linux administration
 * commands. When executed the commands will reconstruct the original
 * user and group information.
 *
 * @package aforms
 * @subpackage lib-sys
 *
 */
class account_migration_c extends account_info_c
{
  /**
   *
   * Object constructor
   *
   * <p>initialization function that calls the various initialization
   * methods. See the constructor in usersgroups.inc.
   *
   * @param string $dir the path to the directory containing the passwd file.
   *    This parameter is required.
   *
   * @return void
   *
   */
   function __construct( $dir )
   {
      parent::__construct($dir);
   }


  /**
   *
   * Generate Linux GroupAdd commands
   *
   * This export script generates a Linux groupadd command that, when
   * executed, has the effect of recreating a group account. The
   * command is built-up as a string and returned to the caller. Note
   * that some of the information found in the group and gshadow files
   * is not exported using this procedure.
   *
   * Following is the help text from the GroupAdd command:
   *
   * <p><pre>
   *  Usage: groupadd [options] group
   *
   *   Options:
   *    -f, --force         force exit with success status if the specified
   *                        group already exists
   *    -r,                 create system account
   *    -g, --gid GID       use GID for the new group
   *    -h, --help          display this help message and exit
   *    -K, --key KEY=VALUE overrides /etc/login.defs defaults
   *    -o, --non-unique    allow create group with duplicate
   *                        (non-unique) GID
   *
   * </pre>
   * @param string $id the id of the group account to be
   *    recreated on another server. This parameter is required.
   *
   * @see $groups
   * @return string
   *
   */
   function gen_groupadd( $id )
   {
      if ( !isset($this->groups[$id]) )
         return;

      if ( ( $this->groups[$id]["gid"] < SYSTEM_MINUSRGID )
        || ( $this->groups[$id]["gid"] > SYSTEM_MAXUSRGID ) )
         return;

      if ( $this->groups[$id]["gid"] <= SYSTEM_MAXSYSGID )
         return;

      $info = $this->groups[$id];
      $cmd = "groupadd --gid {$info['gid']} {$id}";

      return $cmd;
   }


  /**
   *
   * Generate Linux UserAdd commands
   *
   * This export script generates useradd command that, when executed, has
   * the effect of recreating a user account. The command is built-up
   * as a string and returned to the caller.
   *
   * Following is the help text from the UserAdd command:
   *
   * <p><pre>
   *
   *   Usage: useradd [options] LOGIN
   *
   *   Options:
   *    -b, --base-dir BASE_DIR      base directory for the new user account
   *                                 home directory
   *    -c, --comment COMMENT        set the GECOS field for the new user account
   *    -d, --home-dir HOME_DIR      home directory for the new user account
   *    -D, --defaults               print or save modified default useradd
   *                                 configuration
   *    -e, --expiredate EXPIRE_DATE set account expiration date to EXPIRE_DATE
   *    -f, --inactive INACTIVE      set password inactive after expiration
   *                                 to INACTIVE
   *    -g, --gid GROUP              force use GROUP for the new user account
   *    -G, --groups GROUPS          list of supplementary groups for the new
   *                                 user account
   *    -h, --help                   display this help message and exit
   *    -k, --skel SKEL_DIR          specify an alternative skel directory
   *    -K, --key KEY=VALUE          overrides /etc/login.defs defaults
   *    -m, --create-home            create home directory for the new user
   *                                 account
   *    -l,                          do not add user to lastlog database file
   *    -M,                          do not create user's home directory(overrides /etc/login.defs)
   *    -r,                          create system account
   *    -o, --non-unique             allow create user with duplicate
   *                                 (non-unique) UID
   *    -p, --password PASSWORD      use encrypted password for the new user
   *                                 account
   *    -s, --shell SHELL            the login shell for the new user account
   *    -u, --uid UID                force use the UID for the new user account
   *    -Z, --selinux-user SEUSER    use a specific SEUSER for the SELinux user mapping
   *
   * </pre>
   *
   * @param string $id the user id of the user account to be
   *    recreated on another server.  This parameter is required.
   *
   * @see $users
   * @return string
   *
   */
   function gen_useradd( $id )
   {
      if ( !isset($this->users[$id]) )
         return;

      if ( ( $this->users[$id]["uid"] < SYSTEM_MINUSRUID )
        || ( $this->users[$id]["uid"] > SYSTEM_MAXUSRUID ) )
         return;

      $info = $this->users[$id];
      $cmd = "useradd"
           . ($info['gid']    > "" ? " --gid {$info['gid']} "             : "")
           . ($info['uid']    > "" ? " --uid {$info['uid']} "             : "")
           . ($info['dir']    > "" ? " --home-dir \"{$info['dir']}\" "        : "")
           . ($info['ident']  > "" ? " --comment \"{$info['ident']}\" "       : "")
           . ($info['expire'] > "" ? " --expiredate \"{$info['expire']}\" "   : "")
           . ($info['inact']  > "" ? " --inactive \"{$info['inact']}\" "      : "")
           . ($info['groups'] > "" ? " --groups \"{$info['groups']}\" "   : "")
           . ($info['passwd'] > "" ? " --password \"{$info['passwd']}\" " : "")
           . ($info['shell']  > "" ? " --shell \"{$info['shell']}\" "     : "")
           .        "-M {$id}"
           ;

      return $cmd;
   }


  /**
   *
   * Generate Migration Commands
   *
   * This export script calls the methods to generate the user and group
   * account creation commands based on the information in the $users
   * and $groups arrays. The resulting command strings are simply
   * echod to the console. The sysadmin will likely redirect this
   * output to a file for later review. Once the sysadmin has verified
   * that the commands in the script are correct the script can be
   * moved to the target server and executed to recreate the user and
   * group accounts.
   *
   * @param string $id the user id of the user account to be
   *    recreated on another server.  This parameter is required.
   *
   * @see $users
   * @see $groups
   * @see gen_useradd()
   * @see gen_groupadd()
   * @return void
   *
   */
   function migration_commands()
   {
      $hdr_shown = false;
      $userids  = array_keys( $this->users );
      $groupids = array_keys( $this->groups );

      $ids = array();
      foreach ( $userids as $id )  $ids[$id] = true;
      foreach ( $groupids as $id ) $ids[$id] = true;
      $ids = array_keys( $ids );

      foreach ( $ids as $id )
      {
         $grp_cmd = $this->gen_groupadd( $id );
         $usr_cmd = $this->gen_useradd( $id );

         if ( $grp_cmd || $usr_cmd )
         {
            if ( $hdr_shown == false )
            {
               $hdr_shown = true;
               echo "\n#\n#\n# Account dump from {$this->dir} on {$this->host}\n#\n#\n\n";
            }

            echo "# {$id}\n",
                 $grp_cmd ? "{$grp_cmd}\n" : "",
                 $usr_cmd ? "{$usr_cmd}\n" : "",
                 "\n"
                 ;
         }
      }
   }
}

