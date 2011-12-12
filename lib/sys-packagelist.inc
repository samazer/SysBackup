<?php /* -*- mode: php; mode: mmm; coding: utf-8-unix; -*- */
/**
 *
 * System utils - Generate a system package list
 *
 * <b>package_list()</b> is a simple method that calls the distro
 * package management software to emit a list of installed
 * packages. (This code was originally written as part of a server
 * backup solution that saves a list of the installed packages as part
 * of the server state.)
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
 * Redhat command to generate the list of currently installed packages
 *
 */
define( "CMD_REDHAT_PACKAGE_LIST", 
        "/bin/rpm -qa --qf '%{NAME} %{VERSION}-%{RELEASE} %{ARCH}\n' | sort" );

/**
 *
 * Debian command to generate the list of currently installed packages
 *
 */
define( "CMD_DEBIAN_PACKAGE_LIST", "/usr/bin/dpkg --list" );


require_once "general-sysutils.inc";


   /**
    *
    * package_list( $fn = true, $cmd = false )
    *
    * generates a package list for the current system.
    *
    * Currently this function only recognizes the redhat and debian
    * families of distros. For other systems it is necessary to pass
    * the desired command in the $cmd parameter.
    *
    * If $fn = false: returns a huge string (often several hundred
    * kilo bytes in size) containing the list of installed packages on
    * the current server.
    *
    * If $fn = true: creates a temporary file in the system temp
    * folder, writes the package list to that file and returns the
    * name of the file.
    *
    * If $fn contains a file name: opens that file for output and
    * writes the system package list to that file. Returns $fn when
    * done.
    *
    * The $cmd parameter can be used to override the default package
    * generation commands provided by this function (though, obviously
    * it is easier to consolidate package list generation for
    * different operating systems in this module.)
    *
    * Returns false if an error occurs. If a temporary file was
    * created prior to the error the file will be removed. If the
    * caller passed a file name that file will also be removed in the
    * event of an error.
    *
    */
   function package_list( $fn = true, $cmd = false )
   {
      if ( $cmd === false )
      {
         $system = new linux_distro_c();

         switch( $system->family() )
         {
            case "redhat":
               $cmd = CMD_REDHAT_PACKAGE_LIST;
               break;

            case "debian":
               $cmd = CMD_DEBIAN_PACKAGE_LIST;
               break;

            default:
               // can't produce package list - don't know how.
               return false;
         }
      }

      if ( $fn === false )
         return `{$cmd}`;

      if ( $fn === true  )
         $fn = tempnam( sys_get_temp_dir(), "package-list-" );

      $r = 0;
      $params = array( "descriptorspec" => array( 1 => array( "file", $fn, "w" ) ) );

      if ( sysexec( $r, $cmd, $params ) === false )
         return $fn;

      unlink( $fn );
      return true;
   }



