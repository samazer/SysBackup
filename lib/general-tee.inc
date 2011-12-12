<?php /* -*- mode: php; mode: mmm; coding: utf-8-unix; -*- */
/**
 *
 * General tee (write to the screen and/or a screen dump file)
 *
 * utility class to display status messages while logging them for
 * future review
 *
 * extends the logging class as that functionality is often used in
 * simple server maintenance scripts
 *
 * @author Sam Azer <sam@azertech.net>
 * @version 0.01 (alpha)
 * @package server-maint
 * @subpackage lib-sys
 * @license http://www.gnu.org/licenses/gpl.html GPL
 * @copyright 2011 by Sam Azer, All rights reserved
 * @link http://www.azertech.net/
 * @link http://www.samazer.net/about
 *
 */



require_once "general-log.inc";



class teelog_c extends applog_c
{
   var $quiet_flag = false;   // flag true if output should be muted
   var $teefile    = false;   // full path and file name of output log file

   // resource for the open log file or integer value indicating status:
   var $teefp      = -1;      // -1 = not used, -2 = closed


   function __construct( $fn = "" )
   {
      parent::__construct();

      $this->teefile      = $fn;
   }


   function not_quiet( $f = -1 )
   {
      if ( $f === -1 )
         return !$this->quiet_flag;

      return $this->quiet_flag = !$f;
   }


   function notquiet( $f = -1 )
   {
      return $this->not_quiet($f);
   }


   function quiet( $f = -1 )
   {
      if ( $f === -1 )
         return $this->quiet_flag;

      return $this->quiet_flag = $f;
   }


   function teefn( $fn = false )
   {
      if ( $fn )
         $this->teefile = $fn;

      return $this->teefile;
   }


   /**
    *
    * Tee status text to the screen and to a log file
    *
    * This method should be used by all other methods to generate
    * status text. The text will be displayed to the console and also
    * written to a log file for later review. The idea is to keep a
    * record of exactly what was done with each backup so that it can
    * be reviewed in the event of a problem.
    *
    */
   function tee( $msg, $eol = "\n" )
   {
      // undo close operation
      if ( $msg === true )
      {
         $this->teefp = -1;
         return;
      }

      // close the output file if the msg is boolean false
      if ( $msg === false )
      {
         // only if it's already open
         if ( is_resource($this->teefp) )
            if ( fclose( $this->teefp ) === false )
               throw new Exceptin( TEELOG_TXTFATAL . "unable to close the log file" );

         // close the output file
         $this->teefp = -2;
         return false;
      }

      // emit a message
      echo "{$msg}{$eol}";

      // closed until reopened
      if ( $this->teefp === -2 )
         return;

      // don't try to write to the log file until the path/fn is set
      if ( !$this->teefile )
         return;

      // try to open if it's never been opened
      if ( $this->teefp === -1 )
         if ( ($this->teefp = fopen( $this->teefile, "w" )) === false )
            throw new Exception( TEELOG_TXTFATAL . "unable to open the log file for writing ({$this->teefile})" );

      // save a copy of the text to the log file
      if ( fwrite( $this->teefp, $msg . $eol ) === false )
         throw new Exception( TEELOG_TXTFATAL . "unable to write to the log file" );

      // flush often as incomplete log files might be copied
      fflush( $this->teefp );
   }


   function fatal( $msg, $stop_flag = true )
   {
      $this->tee( TEELOG_TXTFATAL . "{$msg}" );

      $posix_uid   = $this->sys_user->uid();
      $posix_gid   = $this->sys_user->gid();
      $posix_user  = $this->sys_user->user();
      $posix_group = $this->sys_user->group();

      $this->tee( TEELOG_TXTFATAL . "starting user:group was {$posix_user}:{$posix_group} ({$posix_uid}:{$posix_gid}), cwd was {$this->starting_cwd}" );

      $this->sys_user->refresh();
      $posix_uid   = $this->sys_user->uid();
      $posix_gid   = $this->sys_user->gid();
      $posix_user  = $this->sys_user->user();
      $posix_group = $this->sys_user->group();
      $cwd         = getcwd();

      $this->tee( TEELOG_TXTFATAL . "current user:group is {$posix_user}:{$posix_group} ({$posix_uid}:{$posix_gid}), cwd is {$cwd}" );

      $this->tee( TEELOG_TXTFATAL . "Backtrace follows:" );
      debug_print_backtrace(DEBUG_BACKTRACE_IGNORE_ARGS);
      $this->tee( TEELOG_TXTFATAL . "stopped " . date( "Y.m.d H:i:s" ) 
                                  . " after " . $this->all_time(true) . " seconds" );

      if ( $stop_flag )
         throw new Exception( $msg );
   }


   function warning( $msg )
   {
      $this->tee( TEELOG_TXTWARNING . "{$msg}" );
   }
}



