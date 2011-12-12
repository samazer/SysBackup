<?php /* -*- mode: php; mode: mmm; coding: utf-8-unix; -*- */
/**
 *
 * General stopwatch funcs
 *
 * utility class to keep track of job times
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


class stopwatch_c
{

   var $all_start_t = false;
   var $all_stop_t  = false;

   var $job_start_t = array(); // array of job start times - by job name
   var $job_stop_t  = array(); // array of job stop times - by job name


   function duration( $tot_secs )
   {
      $fraction = (int)(($tot_secs - (int)$tot_secs) * 10000);
      $hours    = (int)( $tot_secs / 60 / 60);
      $mins     = (int)(($tot_secs - ($hours * 60*60)) / 60);
      $secs     = (int)(($tot_secs - ($hours * 60*60) - ($mins * 60)));

      if ( $hours && $mins )
         return sprintf( "%d:%02d:%02d.%04d", $hours, $mins, $secs, $fraction );

      if ( $mins )
         return sprintf( "%d:%02d.%04d", $mins, $secs, $fraction );

      return sprintf( "%d.%04d", $secs, $fraction );
   }



   /**
    *
    * All Start
    *
    * if the $set_start parameter is true this method will set the
    * start time for the stopwatch to the current time.
    *
    * Returns the time at which the all_start() method was first
    * called with a $set_start parameter of true. Returns false if
    * this was never done.
    *
    */
   function all_start( $set_start = false )
   {
      if ( $set_start == true )
         $this->all_start_t = microtime(true);

      return $this->all_start_t;
   }


   /**
    *
    * All Stop
    *
    * if the $set_stop parameter is true this method will set the stop
    * time for the stopwatch to the current time. It will also loop
    * through the list of lap times and, for any laps that have not
    * already been stopped, will stop them.
    *
    * Returns the time at which the all_stop() method was first
    * called with a $set_stop parameter of true. Returns false if
    * this was never done.
    *
    */
   function all_stop( $set_stop = false )
   {
      if ( $set_stop == true )
      {
         $this->all_stop_t = microtime(true);

         $to_stop = array();
         foreach ( $this->job_stop_t as $lap => $ut )
            if ( $ut === false )
               $to_stop[] = $lap;

         foreach ( $to_stop as $lap )
            $this->job_stop_t[$lap] = microtime(true);
      }

      return $this->all_stop_t;
   }


   /**
    *
    * All Time
    *
    * Returns the current number of seconds between the all start and
    * all stop times for the stopwatch. If the stopwatch is currently
    * running this will represent the number of seconds elapsed since
    * the stopwatch was started. If the stopwatch is not running it
    * will represent the number of seconds between the last start and
    * stop. If the stopwatch never ran the return value will be zero.
    *
    * If the $format parameter is true the result will be formatted to
    * four decimal places.
    *
    */
   function all_time( $format = false )
   {
      if ( ($start_time = $this->all_start(0)) == false )
         $start_time = microtime(true);

      if ( ($stop_time = $this->all_stop(0)) == false )
         $stop_time = microtime(true);

      if ( ($secs = $stop_time - $start_time) < 0 )
         $secs = 0;

      return $format ? number_format( $secs, 4 )
                     : $secs
           ;
   }


   /**
    *
    * Lap Start
    *
    * $lap_name is the name of the lap (usually a task) being timed.
    *
    * if $set_start is true this method sets the current time as the
    * start time for the specified $lap_name.
    *
    * Returns the recorded start time for the specified $lap_name. The
    * stopwatch will use the all_start() time if the lap time was not
    * previously set.
    *
    */
   function lap_start( $lap_name = "", $set_start = false )
   {
      if ( $set_start == true )
         $this->job_start_t[$lap_name] = microtime(true);

      if ( !isset($this->job_start_t[$lap_name]) )
         $this->job_start_t[$lap_name] = $this->all_start(0);

      $this->job_stop_t[$lap_name] = false;
      return $this->job_start_t[$lap_name];
   }


   /**
    *
    * Lap Stop
    *
    * $lap_name is the name of the lap (usually a task) being timed.
    *
    * if $set_stop is true this method sets the current time as the
    * stop time for the specified $lap_name.
    *
    * Returns the recorded stop time for the specified $lap_name. The
    * stopwatch will use the all_stop() time if the lap stop time was
    * not previously set. Otherwise returns false if the lap is still
    * running.
    *
    */
   function lap_stop( $lap_name = "", $set_stop = false )
   {
      if ( $set_stop )
      {
         $this->lap_start( $lap_name, false );
         $this->job_stop_t[$lap_name] = microtime(true);
      }

      return $this->job_stop_t[$lap_name];
   }


   /**
    *
    * Lap Time
    *
    * $lap_name is the name of the lap (usually a task) being timed.
    *
    * Returns the number of seconds elapsed for the specified lap.
    *
    * Returns the recorded stop time for the specified $lap_name. The
    * stopwatch will use the all_stop() time if the lap stop time was
    * not previously set. Otherwise returns false if the lap is still
    * running.
    *
    * If the $format parameter is true the result will be formatted to
    * four decimal places.
    *
    */
   function lap_time( $lap_name = "", $format = false )
   {
      if ( ($stop_time = $this->lap_stop($lap_name,false)) == false )
         $stop_time = microtime(true);

      if ( ($secs = $stop_time - $this->lap_start($lap_name,false)) < 0 )
         $secs = 0;

      return $format ? number_format( $secs, 4 )
                     : $secs
           ;
   }
}



