<?php /* -*- mode: php; mode: mmm; coding: utf-8-unix; -*- */
/**
 *
 * System utils - utility and support for linux systems
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
 * @version 0.01 (alpha)
 * @package aforms
 * @subpackage lib-sys
 * @license http://www.gnu.org/licenses/agpl.html AGPL
 * @copyright 2011 by Sam Azer, All rights reserved
 * @link http://www.azertech.net/
 * @link http://www.samazer.net/about
 *
 *
 */


/**
 *
 * A file that, if it exists, indicates the current OS is Redhat derived
 *
 */
define( "FN_SYS_REDHAT_VERSION", "/etc/redhat-release" );

/**
 *
 * A file that, if it exists, indicates the current OS is Debian derived
 *
 */
define( "FN_SYS_DEBIAN_VERSION", "/etc/debian_version" );



/**
 *
 * A class that tries to determine in which distro it is running.
 *
 * @package aforms
 * @subpackage lib-sys
 *
 */
class linux_distro_c
{
   /**
    *
    * flags set to true when distro-specific info is found.
    *
    */
   var $is_redhat_f;     // true if the redhat version file is found in etc
   var $is_debian_f;     // true if the debian version file is found in etc

   /**
    *
    * __construct()
    *
    * checks for the different distro family files and sets the
    * appropriate flags.
    *
    */
   function __construct()
   {
      $this->is_redhat_f = file_exists( FN_SYS_REDHAT_VERSION );
      $this->is_debian_f = file_exists( FN_SYS_DEBIAN_VERSION );
   }


   /**
    *
    * is_recognized()
    *
    * Returns true if the current distro appears to be one of the
    * distros recognized by this class.
    *
    */
   function is_recognized()
   {
      return $this->is_redhat_f
          || $this->is_debian_f
           ;
   }


   /**
    *
    * family()
    *
    * Returns a distro family name (currently either redhat or debian)
    * based on the flags that were set by the constructor.
    *
    */
   function family()
   {
      if ( $this->is_redhat_f )
         return "redhat";

      if ( $this->is_debian_f )
         return "debian";

      return "unknown";
   }


   /**
    *
    * is_redhat()
    *
    * Returns true if this system appears to be based on redhat
    *
    */
   function is_redhat()
   {
      return $this->is_redhat_f;
   }


   /**
    *
    * is_debian()
    *
    * Returns true if this system appears to be based on debian
    *
    */
   function is_debian()
   {
      return $this->is_debian_f;
   }
}



/**
 *
 * A class that collects basic information about the effective user
 * and group. This info can later be used, for example, when producing
 * error messages.
 *
 * @package aforms
 * @subpackage lib-sys
 *
 */
class effective_user_c
{
   /**
    *
    * information collected by the __constructor() about the current
    * environment and permissions. At the time of this writing the
    * information is intended to be used by code that emits error
    * messages. The idea is to ensure that error messages include
    * information that can be used for debugging.
    *
    */
   var $posix_uid;     // Effective process User ID  on object create
   var $posix_user;    // ID of the effective user on object create
   var $posix_gecos;   // GECOS info array (usually index 0 contains the real name of the user)

   var $posix_gid;     // Effective process Group ID on object create
   var $posix_group;   // Group ID of the effective group on object create


   /**
    *
    * __construct()
    *
    * Collects information about the current environment - including
    * effective uid & gid info.
    *
    */
   function refresh()
   {
      $this->posix_uid = posix_geteuid();
      $this->posix_gid = posix_getegid();

      $info = posix_getpwuid( $this->posix_uid );
      $this->posix_user  = $info["name"];
      $this->posix_gecos = explode( ",", $info["gecos"] );

      $info = posix_getgrgid( $this->posix_gid );
      $this->posix_group = $info["name"];
   }


   function __construct()
   {
      $this->refresh();
   }


   /**
    *
    * whoami()
    *
    * determines the effective uid() at the time of the function
    * call. Note that this may be different from the effective UID at
    * the time of the object instantiation. Converts the UID into a
    * user name and returns the user name. At the time of this writing
    * this information is intended to be used in error messages to
    * facilitate debugging of system scripts.
    *
    */
   function whoami()
   {
      $info = posix_getpwuid( posix_geteuid() );

      return $info["name"];
   }


   /**
    *
    * uid()
    *
    * returns the effective user id number at the time of object instantiation.
    *
    */
   function uid()   { return $this->posix_uid;      }

   /**
    *
    * gid()
    *
    * returns the effective group id number at the time of object instantiation.
    *
    */
   function gid()   { return $this->posix_gid;      }

   /**
    *
    * user()
    *
    * returns the effective user id at the time of object
    * instantiation. Note that this information should not be used in
    * an error message if the current script may have executed a set
    * user id call prior to the execution of the code that experienced
    * the error.
    *
    */
   function user()  { return $this->posix_user;     }

   /**
    *
    * group()
    *
    * returns the effective group id at the time of object
    * instantiation. Note that this information should not be used in
    * an error message if the current script may have executed a set
    * group id call prior to the execution of the code that experienced
    * the error.
    *
    */
   function group() { return $this->posix_group;    }

   /**
    *
    * uid()
    *
    * returns the effective uid at the time of object instantiation.
    *
    */
   function name()  { return $this->posix_gecos[0]; }
}




/**
 *
 * can_hardlink( $target_dir, $source_dir )
 *
 * Brute-force check to see if it is possible to create a hardlink
 * between files in two directories.
 *
 * returns false if either folder does not have read/write
 * access. This can be either because the path is not valid or because
 * the permissions do not allow access for any reason.
 *
 * returns true if the canonical paths of source_dir and
 * target_dir are identical.
 *
 * Otherwise creates a temporary file in the target dir and attempts
 * to hardlink to it from the source dir. Returns true if the attempt
 * is successful and false if not. Temporary files are, of course,
 * removed before returning the result.
 *
 * @param string $target_dir path to the target directory
 * @param string $source_dir path to the source directory
 * @return bool true if it is possible to hardlink files between the source and target directories
 *
 */

function can_hardlink( $target_dir, $source_dir )
{
   // get the canonical path or false if not a valid folder
   if ( !($rp_t = realpath( $target_dir )) )
      return false;

   if ( !($rp_s = realpath( $source_dir )) )
      return false;

   if ( !is_writable( $rp_t ) )
      return false;

   if ( !is_writable( $rp_s ) )
      return false;

   if ( $rp_t == $rp_s )
      return true;

   $t = tempnam( $rp_t, "linktest-" );
   $s = dir_fn( $rp_s, basename( $t ) );

   if ( ($r = @link( $t, $s )) === true )
   {
      unlink( $t );
      unlink( $s );
      return true;
   }

   unlink( $t );
   return $r;
}



/**
 *
 * subfolder_sizes( $dir )
 *
 * Returns an array of relative paths to subfolders containing the
 * following information:
 *
 * - At Index 0: The last date/time of a change in any of the files or
 *   subfolders within the subfolder.
 *
 * - At Index 1: The total number of disk usage bytes used within the
 *   subfolder
 *
 * The DU command is used to compile the information. It returns a
 * single line of results for each subfolder and, at the end, a
 * summary line for the specified $dir showing the last date/time of a
 * change to a file within the folder tree and the total size of the
 * folder tree. Because the $dir is removed from the return results
 * the summary information is usually available at index "" (ie:
 * $r[""] contains a summary record) if $dir is passed with a trailing
 * slash.
 *
 * Returns false if an error occurs.
 *
 * @param string $dir the folder to scan
 * @param string $depth the # of subfolder levels to report in results
 * @param string $du_opts optional options passed to the DU utility
 * @param string $du_bin  optional path to the DU binary
 * @returns array an array of subfolders found, each with associated size and lastmod time info
 *
 */
function subfolder_sizes( $dir, $depth=1, $du_opts = "", $du_bin = false )
{
   if ( $du_bin === false )
      $du_bin  = "/usr/bin/du";

   $cmd = "{$du_bin} --time --block-size=1 --max-depth={$depth} {$du_opts} {$dir}";
   if ( ($du_out = shell_exec($cmd)) === NULL )
      return false;

   $du_out = explode( "\n", trim($du_out) );

   $r = array();
   foreach ( $du_out as $info )
   {
      $x = explode( "\t", $info );
      $r[str_replace( $dir, "", $x[2])] = array( strtotime($x[1]), (int)$x[0] );
   }

   return $r;
}


/**
 *
 * Disk Usage
 *
 * Find out how much disk space is being used by the specified file_ref (usually a folder)
 *
 * @param string $ref absolute or relative reference to a folder or file
 * @return int disk usage in bytes

 */
function disk_usage( $ref )
{
   if ( ($r = subfolder_sizes( $ref, 0 )) === false )
      return false;

   return $r[""][1];
}



/**
 *
 * List folder Items
 *
 * (utility Used by the functions below)
 *
 */
function _list_folder_items( $folder, $folders_only = true )
{
   if ( !file_exists( $folder ) )
      return false;

   if ( chdir( $folder ) === false )
      return false;

   if ( ($list = glob( "*", $folders_only ? GLOB_ONLYDIR : 0 )) === false )
      return false;

   $items = array();
   sort($list);
   foreach( $list as $idx => $item )
   {
      if ( ($t = filemtime($item)) === false )
         return false;

      /* note that items can share a date/time stamp if they were
         programmatically generated.
       */
      $items[$t + $idx] = $item;
   }

   ksort($items);
   return $items;
}


/**
 *
 * Remove Expired Items
 *
 * Procedure:
 *
 * (1) Get a list of the items in the specified folder.
 * (2) Subtract the most recent $max_items items from the list
 * (3) Delete the remaining items from the list, if any.
 *
 * To accomplish this the simple solution is to take a list of the
 * items found in a particular folder and sort them by date. This
 * would put the oldest items at the top of the list and the most
 * recent items at the bottom of the list. 
 *
 * It then becomes a small matter to check the size of the list to
 * determine if it's $max_items or less and return if that test is
 * true. Otherwise the top item in the list can be deleted and the
 * test repeated until the test is true.
 *
 * Returns the number of items removed, zero if none removed or false
 * on error.
 *
 * This function executes "rm -Rf ..." commands and is often used by
 * scripts that run as root. It's hard to imagine a more frightening
 * place for bugs to be found. Therefore, by default, the debug var is
 * true and the commands are displayed but nothing is actually
 * deleted.
 *
 * @param string $folder the folder to scan
 * @param string $max_items the # of items to keep
 * @param bool $debug true to display "rm -Rf ..." commands, false to actually execute them (after extensive testing!!)
 * @param bool $folders_only true to scan folders only - false to scan folders and files
 * @returns int number of folders that were deleted
 *
 */
function remove_expired_items( $folder, $max_items, $debug = true, $folders_only = true )
{
   if ( ($items = _list_folder_items( $folder, $folders_only )) === false )
      return false;

   if ( count( $items ) <= $max_items )
      return 0; // nothing to delete

   $total_deleted = 0;
   while( count($items) > $max_items )
   {
      $i = each( $items );

      $ref = dir_fn( $folder, $i['value'] );
      $cmd = "rm -Rf \"{$ref}\"";

      if ( $debug )
         echo "DEBUG: '{$cmd}'\n";
      else if ( ($r = shell_exec( $cmd )) > "" )
              throw new Exception( "Unexpected output from command '{$cmd}': {$r}" );

      $total_deleted++;
      unset( $items[$i["key"]] );
      reset( $items );
   }

   return $total_deleted;
}



/**
 *
 * List Expired Items
 *
 * Procedure:
 *
 * (1) Get a list of the items in the specified folder.
 * (2) Subtract the most recent $max_items items from the list
 * (3) List the remaining items from the list, if any.
 *
 * @param string $folder the folder to scan
 * @param string $max_items the # of items to keep
 * @param bool $folders_only true to scan folders only - false to scan folders and files
 * @returns array array of expired folder time stamps & sizes by name, ie: $expired["name"] => array( time stamp, disk usage );
 *
 */
function list_expired_items( $folder, $max_items, $folders_only = true )
{
   if ( ($items = _list_folder_items( $folder, $folders_only )) === false )
      return false;

   if ( count( $items ) <= $max_items )
      return array(); // nothing to delete

   $expired = array();
   while( count($items) > $max_items )
   {
      $i = each( $items );
      unset( $items[$i["key"]] );
      reset( $items );

      $filename = $i["value"];
      $stamp    = $i["key"];
      $size     = disk_usage( $filename );
      $expired[$filename] = array( $stamp, $size );
   }

   return $expired;
}


