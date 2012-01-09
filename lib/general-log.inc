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
 * @subpackage lib-sys
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


// throw if we take longer than 10 minutes to lock
define( "APPLOG_LOCK_MAXTIME", 10*60 );


// wait for lock if buffer gets too big 
define( "APPLOG_BUFFER_MINLINES", 100  );

// throw if the buffer gets too big (something wrong)
define( "APPLOG_BUFFER_MAXLINES", 1000 );


/**
 *
 * @package aforms
 * @subpackage lib-sys
 *
 */
class logfile_c
{
   var $disabled_flag = false; // true to disable the logger

   var $logfile       = false; // full path and file name of output log file

   // resource for the open log file or integer value indicating status:
   var $logfp         = -1;    // -1 = not used, -2 = closed

   var $locked_flag   = false; // true if locked 
   var $lock_timeout  = 300;   // max delay before obtaining a lock on the log file
   var $lock_warning  = 15;    // number of seconds between warnings about waiting for the log file lock

   var $buffer        = array(); // buffer used before file is opened or when file is locked for a long time
   var $buf_minlines  = 100;     // after this number of lines we seriously want to lock and flush.
   var $buf_maxlines  = 1000;    // after this number of lines we throw.

   var $fatal_msg_prefix = "";   // txt to prepend to an exception msg


   /**
    *
    * flag true if the logger throws.
    *
    * history teaches that errors thrown by the logger function are
    * often due to a lack of free space on the filesystem or
    * corruption on the filesystem. such situations often result in
    * multiple error catching blocks attempting to log their errors -
    * but the logger then tries to throw because it is not able to
    * function. This infinite loop is caught through the use of the
    * flag here.
    *
    */
   var $oops_i_threw_an_exception = false;



   function __construct( $fn = "", 
                         $timeout = APPLOG_LOCK_MAXTIME, $warning = 15,
                         $bufmin  = APPLOG_BUFFER_MINLINES,
                         $bufmax  = APPLOG_BUFFER_MAXLINES,
                         $errmsg  = TEELOG_TXTFATAL )
   {
      $this->disabled_flag    = false;
      $this->logfile          = $fn;

      $this->oops_i_threw_an_exception = false;
      $this->locked_flag      = false;

      $this->lock_timeout     = $timeout;
      $this->lock_warning     = $warning;

      $this->buffer           = array();
      $this->buf_minlines     = $bufmin;
      $this->buf_maxlines     = $bufmax;

      $this->fatal_msg_prefix = $errmsg;
   }


   private function _err( $txt )
   {
      return "{$this->fatal_msg_prefix}{$txt}";
   }


   function disabled( $flg = -1 )
   {
      if ( $flg === -1 )
         return $this->disabled_flag;

      return $this->disabled_flag = $flg;
   }


   function name( $fn = false )
   {
      if ( $fn )
         $this->logfile = $fn;

      return $this->logfile;
   }


   /**
    *
    * Reset the oops_i_threw_an_exception flag.
    *
    * Most likely this will never be used - there is little that can
    * be done in such cases as the filesystem is either full or
    * corrupted.
    *
    */
   function reset_fatal_error()
   {
      $this->oops_i_threw_an_exception = false;
   }



   function open( $fn = false, $mode = "a+" )
   {
      if ( $fn === false )
         $fn = $this->logfile;
      else $this->logfile = $fn;

      if ( ($this->logfp = fopen( $fn, $mode )) === false )
      {
         $this->oops_i_threw_an_exception = true;
         throw new Exception($this->_err("unable to open {$this->logfile}"));
      };
   }


   function close()
   {
      // only if it's already open
      if ( is_resource($this->logfp) )
      {
         // defensive unlock op in case of disaster as 
         // PHP5 no longer guarantees unlock on close
         @flock( $this->logfp, LOCK_UN );

         if ( fclose( $this->logfp ) === false )
         {
            $this->oops_i_threw_an_exception = true;
            throw new Exception($this->_err("unable to close {$this->logfile}"));
         }
      }

      // close the output file
      $this->logfp = -1;
      return false;
   }



   function is_open()
   {
      return is_resource( $this->logfp );
   }


   function is_locked()
   {
      return $this->lock_flag;
   }



   function lock( $wait_flag = true )
   {
      // obtain an exclusive write lock on the file

      $start = $warning = microtime(true);
      $lock_obtained = false;
      while ( !$lock_obtained )
      {
         if ( flock( $this->logfp, LOCK_EX | LOCK_NB ) == true )
         {
            $lock_obtained = true;
            $this->locked_flag = true;
            break;
         }

         if ( !$wait_flag && (count($this->buffer) < $this->buf_minlines) )
            break;

         if ( $this->lock_timeout <= (microtime(true)-$start) )
            break;

         if ( $this->lock_warning <= (microtime(true)-$warning) )
         {
            echo "waiting ", $this->duration( microtime(true) - $start ), " for a lock on {$this->logfile}\n";
            $warning = microtime(true);
         }

         sleep(1);
      }


      if ( !$lock_obtained && (count($this->buffer) >= $this->buf_maxlines) )
      {
         $this->oops_i_threw_an_exception = true;
         throw new Exception($this->_err("unable to obtain a lock on {$this->logfile} in {$this->lock_timeout} seconds") );
      }


      return $this->locked_flag;
   }



   function unlock()
   {
      if ( flock( $this->logfp, LOCK_UN ) == false )
      {
         $this->oops_i_threw_an_exception = true;
         throw new Exception($this->_err("unable to unlock {$this->logfile}"));
      }

      $this->locked_flag = false;
   }



   function seek( $pos = 0, $from = SEEK_END )
   {
      if ( fseek( $this->logfp, $pos, $from ) !== 0 ) 
      {
         $this->oops_i_threw_an_exception = true;
         @flock( $this->logfp, LOCK_UN );
         throw new Exception($this->_err("unable to seek on {$this->logfile}"));
      }
   }


   function write( $msg, $eol = "\n" )
   {
      // output any text that was buffered before the output file was opened
      if ( count($this->buffer) )
      {
         $x = $this->buffer;
         $this->buffer = array();

         $count = 0;
         foreach ( $x as $m )
            $count += $this->write( $m, "" );

         return $count;
      }


      // output an array of text messages one-by-one
      if ( is_array( $msg ) )
      {
         $count = 0;
         foreach ( $msg as $m )
            $count += $this->write( $m );

         return $count;
      }


      // write a copy of the text to the log file
      $txt   = $msg . $eol;
      $count = strlen( $txt );

      if ( fwrite( $this->logfp, $txt ) === false )
      {
         $this->oops_i_threw_an_exception = true;
         @flock( $this->logfp, LOCK_UN );
         throw new Exception($this->_err("unable to write to {$this->logfile}"));
      }

      // return the number of bytes written
      return $count;
   }


   function flush()
   {
      // flush often as incomplete log files might be copied
      if ( fflush( $this->logfp ) === false )
      {
         $this->oops_i_threw_an_exception = true;
         @flock( $this->logfp, LOCK_UN );
         throw new Exception($this->_err("unable to flush the log file"));
      }
   }


   function _save_to_buffer( $msg, $eol = "\n" )
   {
      if ( count( $this->buffer ) >= $this->buf_maxlines )
      {
         $this->oops_i_threw_an_exception = true;
         @flock( $this->logfp, LOCK_UN );
         throw new Exception($this->_err("log buffer full"));
      }

      $this->buffer[] = "{$msg}{$eol}";
      return false;
   }



   /**
    *
    * append text to the log file
    *
    */
   function append( $msg, $eol = "\n" )
   {
      // close the output file if the msg is boolean false
      if ( $msg === false )
      {
         $this->close();
         return false;
      }

      // don't try to write to the log file until the path/fn is set
      if ( !$this->logfile )
         return $this->_save_to_buffer( $msg, $eol ); // not a real problem - no log file defined

      if ( $this->oops_i_threw_an_exception )
         return $this->_save_to_buffer( $msg, $eol ); // too bad - nothing remaining can be done

      // try to open if it's never been opened
      if ( !is_resource($this->logfp) )
         $this->open();

      // obtain an exclusive write lock and write to the file
      if ( $this->lock(false) ) // false === don't wait too long for the lock
      {
         $this->seek();
         $count = $this->write( $msg, $eol );
         $this->flush();
         $this->unlock();
         return $count;
      }

      return $this->_save_to_buffer( $msg, $eol );
   }
}



/**
 *
 * @package aforms
 * @subpackage lib-sys
 *
 */
class applog_c extends stopwatch_c
{
   var $logfile;

   var $debug                 = false; // general debug flag
   var $debug_params_showhelp = false; // true to show the help strings for each config param

   var $sys_distro;
   var $sys_user;
   var $starting_cwd;

   var $host_name;    // host name shown in the log
   var $app_name;     // app name shown in the log


   function __construct( $fn = "" )
   {
      $this->logfile      = new logfile_c( $fn );

      $this->debug        = false;
      $this->debug_params_showhelp = false;

      $this->sys_distro   = new linux_distro_c();
      $this->sys_user     = new effective_user_c();
      $this->starting_cwd = getcwd();

      $this->host_name    = trim(`hostname --short`);
      $this->app_name     = basename($GLOBALS["argv"][0]);
   }


   function logfile_disabled( $flg = -1 )
   {
      return $this->logfile->disabled($flg);
   }


   function logfn( $fn = false )
   {
      return $this->logfile->name($fn);
   }


   function log_is_open()
   {
      return $this->logfile->is_open();
   }



   /**
    *
    * We want to write log entries in the form:
    *
    * Mon DD HH:MM:SS sys app: message
    *
    * So we need an application name. This method will set or get the
    * application name for use by the logger.
    *
    */
   function application_name( $nam = false )
   {
      if ( $nam )
         $this->app_name = $nam;

      return $this->app_name;
   }


   /**
    *
    * Identical to the application_name() method but sets or gets the
    * short hostname for use by the logger.
    *
    */
   function hostname_short( $nam = false )
   {
      if ( $nam )
         $this->host_name = $nam;

      return $this->host_name;
   }


   function debug( $on_off = "" )
   {
      if ( $on_off !== "" )
         $this->debug = $on_off;

      return $this->debug;
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


   function log_prefix()
   {
      $pid = posix_getpid();

      return date( "M d H:i:s" ) 
           . " {$this->host_name} {$this->app_name}[{$pid}]: "
           ;
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
      if ( $this->logfile->disabled_flag )
         return 0;

      return $this->logfile->append( $this->log_prefix() . $msg, $eol );
   }
}



function backtrace($opts = DEBUG_BACKTRACE_IGNORE_ARGS)
{
   if ( PHP_VERSION_ID > 50302 )
      debug_print_backtrace(DEBUG_BACKTRACE_IGNORE_ARGS);
   elseif ( function_exists( "debug_print_backtrace" ) )
          debug_print_backtrace();
}


