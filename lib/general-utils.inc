<?php /* -*- mode: php; mode: mmm; coding: utf-8-unix; -*- */
/**
 *
 * General Util funcs
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
 * @version 5.0
 * @package aforms
 * @subpackage lib-general
 * @license http://www.gnu.org/licenses/agpl.html AGPL
 * @copyright 2001-2011 by Sam Azer, All rights reserved
 * @link http://www.azertech.net/
 * @link http://www.samazer.net/about
 *
 */


  /**
   *
   * single_chars( $txt, $chr )
   *
   * Reduces multiple occurences of a character to single and returns the
   * resulting text
   *
   * @param string $txt a string containing sequences of $chr characters
   * @return string the same string with each group of $chr characters reduced to one
   *
   * <code>
   *
   * echo single_chars( "blah----blah--blah-blah",   "-" ), "\n";
   * echo single_chars( "blah-blah--blah---blah",    "-" ), "\n";
   * echo single_chars( "blah----blah--blah---blah", "-" ), "\n";
   *
   * // expects:
   * // blah-blah-blah-blah
   * // blah-blah-blah-blah
   * // blah-blah-blah-blah
   *
   * </code>
   */
   function single_chars( $txt, $chr )
   {
      while ( ($r = str_replace( $chr . $chr, $chr, $txt )) != $txt )
         $txt = $r;

      return $r;
   }





  /**
   *
   * single_spaces( txt )
   *
   * Reduces multiple spaces to single spaces and returns the
   * resulting text
   *
   * @param string $txt a string containing sequences of multiple spaces
   * @return string the same string with each group of spaces reduced to one space
   *
   * <code>
   *
   * echo single_chars( "blah    blah  blah blah",   " " ), "\n";
   * echo single_chars( "blah blah  blah   blah",    " " ), "\n";
   * echo single_chars( "blah    blah  blah   blah", " " ), "\n";
   *
   * // expects:
   * // blah blah blah blah
   * // blah blah blah blah
   * // blah blah blah blah
   *
   * </code>
   */
   function single_spaces( $txt )
   {
      return single_chars( $txt, " " );
   }



  /**
   *
   * dir_fn() concatenate a directory and a file name
   *
   * Add a filename to a directory name separated by a forward
   * slash. Returns the concatenated string with double-forward-slash
   * characters reduced to single forward slashes.
   *
   * @param string $dir a folder path with or without a trailing slash
   * @param string $fn  a file name with or without a leading slash
   * @return string the dir and fn concatenated with extra slashes removed
   *
   * <code>
   *
   * echo dir_fn( "/my/folder/", "/myfile" ), "\n";
   * echo dir_fn( "/my/folder",  "myfile"  ), "\n";
   * echo dir_fn( "//my//folder//",    "//myfile"  ), "\n";
   * echo dir_fn( "",    "myfile"  ), "\n";
   * echo dir_fn( "myfile", ""  ), "\n";
   * echo dir_fn( "folder", "file"  ), "\n";
   *
   * // expects:
   * // /my/folder/myfile
   * // /my/folder/myfile
   * // /my/folder/myfile
   * // myfile
   * // myfile
   * // folder/file
   *
   * </code>
   *
   *
   */
   function dir_fn( $dir, $fn = false )
   {
      return single_chars( $dir . ($dir && $fn ? "/" : "") . $fn, "/" );
   }


  /**
   *
   * Begin to capture output via the PHP output buffer code. The
   * Capture Begin function simply calls the PHP ob_start() function.
   *
   * @param none
   * @return none
   *
   */
   function capture_begin()
   {
      ob_start();
   }


  /**
   *
   * End the capture of output text and return the captured text. Also
   * calls the ob_end_clean() method to flush the PHP output buffer.
   *
   * @param none
   * @return string the text that was captured between the begin and end calls.
   *
   * <code>
   *
   * capture_begin();
   * echo "hello, world\n";
   * echo "hello, world\n";
   * echo "hello, world\n";
   * echo capture_end();
   *
   * // expects:
   * // hello, world
   * // hello, world
   * // hello, world
   *
   * </code>
   */
   function capture_end()
   {
      $r = ob_get_contents();
      ob_end_clean();

      return $r;
   }


   function si_suffix($val, $type = "") 
   {
      if ( is_string( $val ) )
      {
         $val = trim($val);

         $suffix = strtolower(substr($val,-1,1));
         if ( $suffix == 'b' )
            $suffix = strtolower(substr($val,-2,1));

         switch($suffix) 
         {
            case 'g':
               $val *= 1024;
            case 'm':
               $val *= 1024;
            case 'k':
               $val *= 1024;
         }

         return $val;
      }

      if ( !is_integer( $val ) && !is_long( $val ) && !is_float( $val ) )
         return "error";

      $exp = 1;
      if ( $val > 1024 ) { $val /= 1024; $exp *= 3; }
      if ( $val > 1024 ) { $val /= 1024; $exp *= 3; }
      if ( $val > 1024 ) { $val /= 1024; $exp *= 3; }

      if ( $exp == 1 )
         return "{$val}{$type}";

      $val = number_format( $val, 0 );

      if ( $exp == 3 )
         return "{$val}Ki{$type}";

      if ( $exp == 9 )
         return "{$val}Mi{$type}";

      if ( $exp == 27 )
         return "{$val}Gi{$type}";

      return "{$val}{$type}?";
   }



   // this version is for console apps - no HTML codes emitted.
   function phpval2str( $k, $depth=false )
   {
          if ( is_null($k) )
             return "[null]";
      elseif ( $k === false )
             return "bool(false)";
      elseif ( $k === true  )
             return "bool(true)";
      elseif ( is_array($k) )
             return ( $depth ? _phpval2str( $k, --$depth ) : "[array]" );
      elseif ( is_object($k) )
             return "obj(" . get_class( $k ) . ")";
      elseif ( is_float($k) )
             return "float($k)";
      elseif ( is_int($k) )
             return "int($k)";
      elseif ( is_string($k) )
             return $k === "" ? "string(blank)" : "\"{$k}\"";
      elseif ( is_resource($k) )
             return "\"{$k}\"";

      return "??<{$k}>??";
   }


   function _glob_match( $base, $folder, $inc, $exclude = array(), $depth = false, $flags = false )
   {
      $depth = $depth-1;

      $fldr = dir_fn($base, $folder);
      if ( chdir( $fldr ) !== true )
         throw new Exception( "can't chdir to {$fldr}" );

      $result = array();

      if ( $flags === false )
         $found = glob( $inc );
      else $found = glob( $inc, $flags );

      foreach ( $found as $fn )
      {
         $excluded = false;

         if ( ($fn == ".") || ($fn == "..") )
            continue;

         foreach ( $exclude as $ex )
         {
            if ( substr( strtolower( $ex ), 0, 5 ) == "regx:" )
            {
               $ex = substr( $ex, 5 );

               if ( ($r = preg_match( $ex, $fn )) === false )
                  throw new Exception( "preg_match( '{$ex}', '{$fn}' ) error" );

               if ( $r > 0 )
               {
                  $excluded = true;
                  break;
               }
            }
              else if ( strpos( $fn, $ex ) !== false )
                   {
                      $excluded = true;
                      break;
                   }
         }


         $fldr = dir_fn( $folder, $fn );

         if ( !$excluded )
            if ( !isset( $result[$fldr] ) )
            {
               $result[$fldr] = true;

               if ( $depth > 0 )
               {
                  if ( is_dir( dir_fn( $base, $fldr ) ) )
                     $result = array_merge( $result, 
                                            _glob_match( $base, $fldr, $inc, $exclude, $depth, $flags ) );
               }
            }
      }

      return $result;
   }


   function glob_match( $base, $include, $exclude = array(), $depth = 1, $flags = false )
   {
      if ( is_string( $include ) )
         $include = array( $include );

      if ( is_string( $exclude ) )
         $exclude = array( $exclude );

      if ( $depth < 1 )
         $depth = 1;

      $results = array();
      foreach ( $include as $inc )
         $results = array_merge( $results, _glob_match( $base, "", $inc, $exclude, $depth, $flags ) );

      return array_keys($results);
   }



   /**
    *
    * find_prefixed_value( $haystack, $needle );
    *
    * In a string containing useful information such as: "eth0 Link
    * encap:Ethernet HWaddr 00:19:5b:82:9a:8f" we want to find "link
    * encap:" and return the text that follows (Ethernet HW...) or
    * false if we fail.
    *
    */
   function find_prefixed_value( $haystack, $needle )
   {
      if ( ($p = stripos( $haystack, $needle )) !== false )
         return substr( $haystack, $p + strlen($needle) );

      return false;
   }



   /**
    *
    * find_array_element_text( $list, $needle );
    *
    * In an array of strings, return the key associated with a value
    * in which the needle is found (or false if nothing is found.)
    *
    */
   function find_array_element_text( $list, $needle )
   {
      foreach ( $list as $key => $val )
         if ( stripos( $val, $needle ) !== false )
            return $key;

      return false;
   }



  /**
   *
   * get_temp_dir()
   *
   * Returns a working temp folder path
   *
   */
   function get_temp_dir()
   {
      if ( function_exists('sys_get_temp_dir') )
         if ( ($tmp = sys_get_temp_dir()) > "" )
            if ( file_exists( $tmp ) && is_writable( $tmp ) )
               return $tmp;

      $known_envs = array( "TMP", "TEMP", "TMPDIR", "TEMPDIR" );
      foreach ( $known_envs as $env )
         if ( ($tmp = getenv($env)) > "" )
           if ( file_exists( $tmp ) && is_writable( $tmp ) )
               return $tmp;

      $known_tmps = array( "/tmp", "/var/tmp" );
      foreach ( $known_tmps as $tmp )
         if ( file_exists( $tmp ) && is_writable( $tmp ) )
            return $tmp;

      return false;
   }


  /**
   *
   * get_temp_filename()
   *
   * Returns a temp file name
   *
   */
   function get_temp_filename( $prefix )
   {
      $dir = get_temp_dir();
      return tempnam( $dir, $prefix );
   }


