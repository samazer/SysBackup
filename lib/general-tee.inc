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
 * @package aforms
 * @subpackage lib-sys
 * @license http://www.gnu.org/licenses/gpl.html GPL
 * @copyright 2011 by Sam Azer, All rights reserved
 * @link http://www.azertech.net/
 * @link http://www.samazer.net/about
 *
 */



require_once "general-log.inc";



/**
 *
 * @package aforms
 * @subpackage lib-sys
 *
 */
class teelog_c extends applog_c
{
   var $quiet_flag = false;   // flag true if output should be muted
   var $teefile;              // logfile object for the output file


   function __construct( $fn = "" )
   {
      parent::__construct();

      $this->teefile = new logfile_c($fn);
      $this->quiet_flag = false;
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


   private function _check_teefile()
   {
      if ( is_object( $this->teefile ) )
         return;

      die( "\n\nobject of type teelog_c() not properly constructed;\n"
         . "did you forget to call the parent constructor in your application?\n\n" );
   }


   function teefn( $fn = false )
   {
      $this->_check_teefile();
      return $this->teefile->name($fn);
   }


   /**
    *
    * Tee status text to the screen and to a display log file
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
      // output an array of text messages one-by-one
      if ( is_array( $msg ) )
      {
         foreach ( $msg as $m )
            echo "{$m}\n";

         return $this->teefile->append( $msg, "\n" );
      }

      echo "{$msg}{$eol}"; // emit a message

      $this->_check_teefile();
      $this->teefile->append( $msg, $eol );
   }


   function fatal( $msg, $stop_flag = true )
   {
      capture_begin();

      // output an array of text messages one-by-one
      if ( is_array( $msg ) )
      {
         foreach ( $msg as $m )
            $this->tee( TEELOG_TXTFATAL . "{$m}" );

      } 
        else $this->tee( TEELOG_TXTFATAL . "{$msg}" );

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

      backtrace();

      $this->tee( TEELOG_TXTFATAL . "stopped " . date( "Y.m.d H:i:s" ) 
                                  . " after " . $this->all_time(true) . " seconds" );

      $r = capture_end();

      $this->log( explode( "\n", $r ) );

      echo $r;

      if ( $stop_flag )
      {
         if ( is_array( $msg ) )
         {
            $txt = "";
            foreach ( $msg as $t )
               $txt .= "{$t}\n";

            $msg = $txt;
         }

         throw new Exception( $msg );
      }
   }


   function warning( $msg )
   {
      $msg = TEELOG_TXTWARNING . "{$msg}";

      $this->log( $msg );
      $this->tee( $msg );
   }
}



