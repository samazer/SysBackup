<?php /* -*- mode: php; mode: mmm; coding: utf-8-unix; -*- */
/**
 *
 * System utils - utility function to exec a process
 *
 * WARNING: There are a million different ways this code might be
 *          wrong, a million ways it might break in various situations
 *          and a million kinds of errors it might not handle well.
 * 
 * It's easy to get this code to work for any given set of normal
 * working conditions - it's another thing entirely to be confident
 * that it handles a wider variety of situations - especially errors.
 *
 * The fact that it works so well in general should not be considered
 * relevant as it's the error cases that are important!  REVIEW THIS
 * CODE CAREFULLY to verify that it is suitable for your intended
 * purpose. If you have a problem with this code PLEASE TELL THE
 * AUTHOR (see below) so that the code can be adjusted appropriately.
 *
 * <b>sysfilter()</b> is an exec()-like utility which can send a
 * string to the std input stream of a process and filter the std
 * output through a second process. The error streams of the two
 * processes are saved, along with their exit codes. sysfilter() usage
 * is similar to sysexec() usage as follows below.
 *
 * <b>sysexec()</b> is an exec()-like utility which can send a
 * string to the std input stream of a process and save the std output,
 * error streams and exit code.
 *
 * <p>Executes the system command in $cmd, optionally using additional
 * information that can be provided through the $params
 * array. sysexec() is a wrapper for the PHP proc_open()
 * function. See the proc_open() documentation for details related
 * to this and related functions:
 *
 * <p>http://www.php.net/manual/en/function.proc-open.php
 *
 * <p>By default the sysexec() command will setup three pipes for the
 * process to be executed: stdin, stdout and stderr. By default the
 * current working directory is used for the process and the
 * environment of the current process is used for the new
 * process. The stdin stream is closed immediately after the
 * process starts. These defaults can be changed by passing
 * parameters through the $params array, as follows:
 * 
 * <pre>
 *
 *    $params["descriptorspec"] = array( 0 => array( "pipe", "r" ), ... );
 *   
 *       Allows the caller to override the stdin, out and error
 *       streams and/or create additional input and output streams
 *       for the called process. See the descriptorspec
 *       documentation in the above-mentioned manual page for
 *       additional information on this parameter.
 *    
 *    $params["stdin"] = "a string to be sent to the process through stdin";
 *    
 *       Allows the caller to send text through the stdin pipe to
 *       the exec'd process.
 *    
 *    $params["cwd"] = "/home/demo/public/";
 *    
 *       Allows the caller to change the current working directory
 *       for the exec'd process.
 *    
 *    $params["env"] = array( "MyEnvVar" => "TheValue", ... );
 *    
 *       Allows the caller to provide a set of environment
 *       variables for the exec'd process.
 *    
 *    $params["other_options"] = array( ... );
 *   
 *       Allows the caller to pass "other_options" to the PHP
 *       proc_open() function as described in the PHP manual page
 *       (see url above.)
 * 
 * </pre>
 *
 * <p>The sysexec() function fills the $r array with a variety of
 * pieces of information collected prior to the call to PHP's
 * proc_close() function. This information is:
 *
 * <pre>
 *
 *    $r["command"]     string  The $cmd being exec'd 
 *    $r["pid"]         int     The process ID (***usually from the shell)
 *    $r["running"]     bool    flag - see the proc_open() manual page
 *    $r["signaled"]    bool    flag - see the proc_open() manual page
 *    $r["stopped"]     bool    flag - see the proc_open() manual page
 *    $r["exitcode"]    int     The exit code (***usually from the shell)
 *    $r["termsig"]     int     signal - see the proc_open() manual page
 *    $r["stopsig"]     int     signal - see the proc_open() manual page
 *    $r["pipe_stdin"]  bool    flag true if the stdin pipe was found
 *    $r["pipe_stdout"] bool    flag true if the stdout pipe was found
 *    $r["pipe_stderr"] bool    flag true if the stderr pipe was found
 *    $r["stdout"]      string  Process output on the stdout pipe
 *    $r["stderr"]      string  Process output on the stderr pipe
 *    $r["proc_close"]  int     The value returned by PHP's proc_close() function
 *
 * </pre>
 *
 * <p>NOTE: *** The proc_open() function will usually execute a
 *              shell which will then execute the $cmd command. As
 *              a result the pid and exitcode may not contain
 *              meaningful information - they often refer to the
 *              shell process. See the PHP proc_open() manual page
 *              (in the comments section) for hints (ie: exec,) on
 *              how to get the desired process information. (NOTE
 *              THAT COMMENTS SUGGESTING PID+1 are wrong as a
 *              server spawns many processes very quickly and their
 *              PID's can't be assumed to be sequential.)
 *
 * <p>Returns false if PHP's proc_open() function succeeds. Returns
 * true if proc_open() failed. Note that the true or false return
 * information provded by the PHP proc_open() function only
 * indicates that PHP was able to exec a process. This says nothing
 * about the success or failure of the command in $cmd.
 *
 *
 * TODO: What I really needed is a class that can run a process and
 *       report on the status of that process. This would allow me to
 *       run multiple processes simulaneously.  Convert this to a
 *       class that is able to return process info. Change the
 *       functions so that they instantiate objects of the class and
 *       wait for the associated processes to finish.
 *
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


define( "FILTERBUFFER_SIZE", 128*1024 );



   /**
    *
    * bool sysexec( &$r, $cmd, $params = array() )
    *
    * Executes the system command in $cmd, optionally using additional
    * information that can be provided through the $params
    * array. sysexec() is a wrapper for the PHP proc_open()
    * function. See the proc_open() documentation for details related
    * to this and related functions:
    *
    * http://www.php.net/manual/en/function.proc-open.php
    *
    * Returns false if PHP's proc_open() function succeeds. Returns
    * true if proc_open() failed. Note that the true or false return
    * information provded by the PHP proc_open() function only
    * indicates that PHP was able to exec a process. This says nothing
    * about the success or failure of the command in $cmd.
    *
    */
   function sysexec( &$r, $cmd, $params = array() )
   {
      $r      = $pipes  = array();
      $stdin  = $stdout = $stderr = "";

      // create the default descriptor spec
      $descriptorspec = array( 0 => array( "pipe", "r" ),
                               1 => array( "pipe", "w" ),
                               2 => array( "pipe", "w" ),
                             );

      // allow the caller to overwrite and extend the basic spec
      if ( isset( $params["descriptorspec"] ) )
         foreach ( $params["descriptorspec"] as $desc_num => $desc_info )
            $descriptorspec[$desc_num] = $desc_info;

      // allow caller to pass additional information:

      // text provided to the process as through the standard input pipe
      $stdin = isset( $params["stdin"] ) ? $params["stdin"] : "";

      // The current working dir; default is the cwd of the current process
      $cwd   = isset( $params["cwd"] )   ? $params["cwd"]   : NULL; 

      // The process environment variables; default is the vars provided to the current proc.
      $env   = isset( $params["env"] )   ? $params["env"]   : NULL; 

      // Other Options; default is (none)
      $other_options = isset( $params["other_options"] ) ? $params["other_options"] : array();


      if ( is_resource( $proc_resource = proc_open( $cmd, $descriptorspec, $pipes,
                                                    $cwd, $env, $other_options ) )   )
      {
         if ( ($found_stdin = isset( $pipes[0] ) && is_resource( $pipes[0] )) == true )
         {
            fwrite( $pipes[0], $stdin );
            fclose( $pipes[0] );
         }

         if ( ($found_stdout = isset( $pipes[1] ) && is_resource( $pipes[1] )) == true )
            $stdout = stream_get_contents( $pipes[1] );

         if ( ($found_stderr = isset( $pipes[2] ) && is_resource( $pipes[2] )) == true )
            $stderr = stream_get_contents( $pipes[2] );

         $r = proc_get_status( $proc_resource );

         $r["pipe_stdin"]  = $found_stdin;
         $r["pipe_stdout"] = $found_stdout;
         $r["pipe_stderr"] = $found_stderr;

         $r["stdout"]      = $stdout;
         $r["stderr"]      = $stderr;

         $r["proc_close"]  = proc_close( $proc_resource );

         // no overtly obvious problem so return false
         return false;
      }

      // problem: process didn't start for unknown reason
      return true;
   }




   /**
    *
    * bool sysfilter( &$r, $cmd, $filter, $params = array() )
    *
    * Executes the system command in $cmd, optionally using additional
    * information that can be provided through the $params array. Also
    * executes the command in $filter, with stdout piped from the $cmd
    * process to the $filter process. 
    *
    * Output $r is an array containing two arrays of information - one
    * for the $cmd process and another for the $filter process.
    *
    * sysfilter() is a wrapper for the PHP proc_open() function. See
    * the proc_open() documentation for details related to this and
    * related functions.
    *
    * Returns false if PHP's proc_open() function (both calls)
    * succeed. Returns true if one of the proc_open() calls
    * failed. Note that the true or false return information provded
    * by the PHP proc_open() function only indicates that PHP was able
    * to exec both processes. This says nothing about the success or
    * failure of the command in $cmd or the $filter command.
    *
    */
   function sysfilter( &$r, $cmd, $filter, $params = array() )
   {
      $r      = $cmd_pipes = $filter_pipes = array();
      $stdin  = $stdout    = $stderr       = "";
      $buffer_size = FILTERBUFFER_SIZE;

      // create the default descriptor spec (for $cmd)
      $cmd_descriptorspec = array( 0 => array( "pipe", "r" ),
                                   1 => array( "pipe", "w" ),
                                   2 => array( "pipe", "w" ),
                                 );

      // allow the caller to overwrite and extend the basic spec
      if ( isset( $params["cmd_descriptorspec"] ) )
         foreach ( $params["cmd_descriptorspec"] as $desc_num => $desc_info )
            $cmd_descriptorspec[$desc_num] = $desc_info;

      // create the default descriptor spec (for $filter)
      $filter_descriptorspec = array( 0 => array( "pipe", "r" ),
                                      1 => array( "pipe", "w" ),
                                      2 => array( "pipe", "w" ),
                                    );

      // allow the caller to overwrite and extend the basic spec
      if ( isset( $params["filter_descriptorspec"] ) )
         foreach ( $params["filter_descriptorspec"] as $desc_num => $desc_info )
            $filter_descriptorspec[$desc_num] = $desc_info;

      // allow caller to pass additional information:

      // text provided to the process as through the standard input pipe
      $stdin = isset( $params["stdin"] ) ? $params["stdin"] : "";

      // The current working dir; default is the cwd of the current process
      $cwd   = isset( $params["cwd"] )   ? $params["cwd"]   : NULL; 

      // The process environment variables; default is the vars provided to the current proc.
      $env   = isset( $params["env"] )   ? $params["env"]   : NULL; 

      // Other Options; default is (none)
      $cmd_other_options    = isset( $params["cmd_other_options"] )    ? $params["cmd_other_options"]    : array();
      $filter_other_options = isset( $params["filter_other_options"] ) ? $params["filter_other_options"] : array();

      // buffer size
      $buffer_size = isset( $params["buffer_size"] ) ? $params["buffer_size"] : $buffer_size;


      if ( is_resource( $cmdproc_resource = proc_open( $cmd, $cmd_descriptorspec, $cmd_pipes,
                                                       $cwd, $env, $cmd_other_options ) )   )
      {
         if ( ($cmdfound_stdin = isset( $cmd_pipes[0] ) && is_resource( $cmd_pipes[0] )) == true )
         {
            fwrite( $cmd_pipes[0], $stdin );
            fclose( $cmd_pipes[0] );
         }

         $cmdfound_stdout = isset( $cmd_pipes[1] ) && is_resource( $cmd_pipes[1] );

         if ( is_resource( $filterproc_resource = proc_open( $filter, $filter_descriptorspec, $filter_pipes,
                                                             $cwd, $env, $filter_other_options ) )   )
         {
            $filterfound_stdin = isset( $filter_pipes[0] ) && is_resource( $filter_pipes[0] );

            // @todo: possibility of a pipe error not detected here
            while ( !feof($cmd_pipes[1]) )
               fwrite( $filter_pipes[0], fgets( $cmd_pipes[1], $buffer_size ) );

            fclose( $filter_pipes[0] );
         }

         if ( ($cmdfound_stderr = isset( $cmd_pipes[2] ) && is_resource( $cmd_pipes[2] )) == true )
            $stderr = stream_get_contents( $cmd_pipes[2] );

         $r[0] = proc_get_status( $cmdproc_resource );

         $r[0]["pipe_stdin"]  = $cmdfound_stdin;
         $r[0]["pipe_stdout"] = $cmdfound_stdout;
         $r[0]["pipe_stderr"] = $cmdfound_stderr;

         $r[0]["stdout"]      = $stdout;
         $r[0]["stderr"]      = $stderr;

         $r[0]["proc_close"]  = proc_close( $cmdproc_resource );

         unset($stdout);
         unset($stderr);

         if ( ($filterfound_stdout = isset( $filter_pipes[1] ) && is_resource( $filter_pipes[1] )) == true )
            $stdout = stream_get_contents( $filter_pipes[1] );

         if ( ($filterfound_stderr = isset( $filter_pipes[2] ) && is_resource( $filter_pipes[2] )) == true )
            $stderr = stream_get_contents( $filter_pipes[2] );

         $r[1] = proc_get_status( $filterproc_resource );

         $r[1]["pipe_stdin"]  = $filterfound_stdin;
         $r[1]["pipe_stdout"] = $filterfound_stdout;
         $r[1]["pipe_stderr"] = $filterfound_stderr;

         $r[1]["stdout"]      = $stdout;
         $r[1]["stderr"]      = $stderr;

         $r[1]["proc_close"]  = proc_close( $filterproc_resource );
         // no overtly obvious problem so return false
         return false;
      }

      // problem: process didn't start for unknown reason
      return true;
   }


