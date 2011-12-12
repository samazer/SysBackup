<?php /* -*- mode: php; mode: mmm; coding: utf-8-unix; -*- */
/**
 *
 * General logging class
 *
 * utility class to log messages to a log file
 *
 * extends stopwatch as that functionality is often used in simple
 * server maintenance scripts
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
 */


define( "TEELOG_TXTFATAL", "***Fatal: " );
define( "TEELOG_TXTWARNING", "***Warning: " );


require_once "general-utils.inc";
require_once "general-stopwatch.inc";
require_once "general-sysutils.inc";


class applog_c extends stopwatch_c
{
   var $debug                 = false; // general debug flag
   var $debug_params_showhelp = false; // true to show the help strings for each config param
   var $logfile               = false; // full path and file name of output log file

   // resource for the open log file or integer value indicating status:
   var $logfp                 = -1;    // -1 = not used, -2 = closed

   var $sys_distro;
   var $sys_user;
   var $starting_cwd;


   function __construct( $fn = "" )
   {
      $this->logfile      = $fn;

      $this->sys_distro   = new linux_distro_c();
      $this->sys_user     = new effective_user_c();
      $this->starting_cwd = getcwd();
   }


   function logfn( $fn = false )
   {
      if ( $fn )
         $this->logfile = $fn;

      return $this->logfile;
   }


   function debug( $on_off = "" )
   {
      if ( $on_off !== "" )
         $this->debug = $on_off;

      return $this->debug;
   }


   /**
    *
    * log status text to the log file only
    *
    * This method is identical to the tee() method except that it logs
    * only to the log file.
    *
    */
   function log( $msg, $eol = "\n" )
   {
      // undo close operation
      if ( $msg === true )
      {
         $this->logfp = -1;
         return;
      }

      // close the output file if the msg is boolean false
      if ( $msg === false )
      {
         // only if it's already open
         if ( is_resource($this->logfp) )
            if ( fclose( $this->logfp ) === false )
               throw new Exception( TEELOG_TXTFATAL . "unable to close the log file" );

         // close the output file
         $this->logfp = -2;
         return false;
      }

      // closed until reopened
      if ( $this->logfp === -2 )
         return;

      // don't try to write to the log file until the path/fn is set
      if ( !$this->logfile )
         return;

      // try to open if it's never been opened
      if ( $this->logfp === -1 )
         if ( ($this->logfp = fopen( $this->logfile, "w" )) === false )
            throw new Exception( TEELOG_TXTFATAL . "unable to open the log file for writing ({$this->logfile})" );

      // save a copy of the text to the log file
      if ( fwrite( $this->logfp, $msg . $eol ) === false )
         throw new Exception( TEELOG_TXTFATAL . "unable to write to the log file" );

      // flush often as incomplete log files might be copied
      fflush( $this->logfp );
   }


   function stamp( $space = false )
   {
      return ($space ? " " : "")
           . date( "Y.m.d H:i:s" )
           . ($space ? " " : "")
           ;
   }


   function status_prefix()
   {
      return date( "i:s: " )
           ;
   }
}



