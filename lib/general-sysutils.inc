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
 * @subpackage lib-general
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
 * @package server-maint
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
 * @package server-maint
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


