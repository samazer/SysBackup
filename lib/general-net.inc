<?php /* -*- mode: php; mode: mmm; coding: utf-8-unix; -*- */
/**
 *
 * General Network funcs
 *
 * <p><b>Copyright</b>
 *
 * <p>Copyright (c) 2011 by Sam Azer, All Rights Reserved.
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
 * @version 0.01a
 * @package aforms
 * @subpackage lib-general
 * @license http://www.gnu.org/licenses/agpl.html AGPL
 * @copyright 2001-2011 by Sam Azer, All rights reserved
 * @link http://www.azertech.net/
 * @link http://www.samazer.net/about
 *
 */


require_once "general-utils.inc";



/**
 * Network Device List
 *
 * An array containing a list of the network devices in /proc/net/dev. 
 *
 */
static $network_device_list = NULL;


/**
 * Network Device List
 *
 * An array containing configuration info returned by ifconfig for each device
 *
 */
static $network_device_info = NULL;



  /**
   *
   * network_device_stats() 
   *
   * Returns the contents of /proc/net/dev as an array. If a specific
   * device is requested then only the metrics for that device are
   * returned. 
   *
   * The result of microtime(true) is added at the end of each metric
   * array as a time stamp for the data. See the detailed comment in
   * the code for an example of the layout of the array.
   *
   */

   function network_device_stats( $specific_device = false )
   {
     /* As of this writing, the following code produces an array that
        looks similar to the array below. It also contains an element
        called "raw" for each device that contains an array of the 15
        metrics and two elements, 0 and 1, that contain the same info
        as "receive" and "transmit"

        $dev = array( netdev => array( "receive" => array(  "bytes" => i, 
                                                            "packets" => i,
                                                            "errs" => i,
                                                            "drop" => i,
                                                            "fifo" => i,
                                                            "frame" => i,
                                                            "compressed" => i,
                                                            "multicast" => i,
                                                            "stamp" => microtime(true),
                                                         ),
                                       "transmit" => array( "bytes",
                                                            "packets",
                                                            "errs",
                                                            "drop",
                                                            "fifo",
                                                            "colls",
                                                            "carrier",
                                                            "compressed",
                                                            "stamp" => microtime(true),
                                                          ),
                                     ),
                      ...,
                    );
      */

      $stamp = microtime(true);
      $txt   = file_get_contents( "/proc/net/dev" );
      $txt   = explode( "\n", trim(single_spaces( $txt )) );

      static $idxs;

      if ( !is_array( $idxs ) )
      {
         $hdr     = str_replace( "|bytes", " | bytes", $txt[1] );
         $hdr     = substr( str_replace( " face ", "", $hdr ), 3 );
         $idxs    = explode( "\n", str_replace( " | ", "\n", $hdr ));
         $idxs[0] = explode( " ", $idxs[0] );
         $idxs[1] = explode( " ", $idxs[1] );
         $idxs[2] = explode( " ", str_replace( " | ", " ", $hdr ));
      }

      unset( $txt[0] );
      unset( $txt[1] );

      $r = array();
      foreach( $txt as $t )
      {
         $ta = explode( ":", trim($t) );

         if ( $specific_device !== false && $ta[0] != $specific_device )
            continue;

         $device = $ta[0];
         $info   = explode( " ", trim($ta[1]) );

         for( $i=0; $i < count( $info ); $i++ )
         {
            $dir_flag  = $i >= count( $idxs[0] );
            $index     = $dir_flag ? $i - count( $idxs[0] )
                                   : $i;
            $index_txt = $idxs[$dir_flag][$index];
            $direction = $dir_flag ? "transmit"
                                   : "receive";

            $r[$device]["raw"][$i]              = (int)$info[$i];
            $r[$device][$dir_flag][$index]      = (int)$info[$i];
            $r[$device][$direction][$index_txt] = (int)$info[$i];
         }

         $r[$device]["raw"][] = $stamp;
         $r[$device][0][] = $stamp;
         $r[$device][1][] = $stamp;
         $r[$device]["receive"]["stamp"] = $stamp;
         $r[$device]["transmit"]["stamp"] = $stamp;
      }

      if ( $specific_device !== false )
         return $r[$specific_device];

      global $network_device_list;
      if ( !is_array($network_device_list) )
         $network_device_list = array_keys( $r );

      return $r;
   }



   function network_devices()
   {
      global $network_device_list;
      if ( !is_array($network_device_list) )
         network_device_stats();

      return $network_device_list;
   }
      



   /**
    *
    * _ifconfig( device )
    *
    * Returns an array containing the data returned by a call to
    * ifconfig. 
    *
    */
   function _ifconfig( $dev, $bin = "/sbin/ifconfig" )
   {
     /**
      results array example:

          array(1) 
          { 
             ["eth0"]=> array(22) { ["MACaddr"]           => string(17) "00:19:5b:82:9a:8f"
                                    ["LinkEncapsulation"] => string(8) "Ethernet"
                                    ["isUP"]              => bool(true)
                                    ["isBROADCAST"]       => bool(true)
                                    ["isRUNNING"]         => bool(true)
                                    ["isMULTICAST"]       => bool(true)
                                    ["Metric"]            => string(1) "1"
                                    ["MTU"]               => string(4) "1500"
                                    ["RXPackets"]         => string(7) "5018323"
                                    ["RXErrors"]          => string(1) "0"
                                    ["RXDropped"]         => string(1) "0"
                                    ["RXOverruns"]        => string(1) "0"
                                    ["RXFrame"]           => string(1) "0"
                                    ["TXPackets"]         => string(7) "3688571"
                                    ["TXErrors"]          => string(1) "0"
                                    ["TXDropped"]         => string(1) "0"
                                    ["TXOverruns"]        => string(1) "0"
                                    ["TXCarrier"]         => string(1) "0"
                                    ["Collisions"]        => string(1) "0"
                                    ["Txqueuelen"]        => string(4) "1000"
                                    ["Interrupt"]         => string(2) "17"
                                    ["BaseAddress"]       => string(6) "0xac00"
                                 }
          }
      */

      $result = array();

      $txt = explode( "\n", `{$bin} {$dev}` );

      if ( ($x = find_array_element_text( $txt, "HWaddr" )) !== false )
      {
         $x = $txt[$x];
         $x = explode( "HWaddr", $x );
         $result["MACaddr"] = trim( $x[1] ); // Media Access Control Address

         if ( ($p = find_prefixed_value( $x[0], "link encap:" )) !== false )
            $result["LinkEncapsulation"] = trim($p);

         $x = $txt[find_array_element_text( $txt, "MTU" )];
         $result["isUP"]        = (strpos( $x, "UP"        ) !== false);
         $result["isBROADCAST"] = (strpos( $x, "BROADCAST" ) !== false);
         $result["isRUNNING"]   = (strpos( $x, "RUNNING"   ) !== false);
         $result["isMULTICAST"] = (strpos( $x, "MULTICAST" ) !== false);

         if ( ($p = find_prefixed_value( $x, "Metric:" )) !== false )
            $result["Metric"] = trim($p);

         if ( ($p = stripos( $x, "Metric:" )) !== false )
            $x = substr( $x, 0, $p );

         if ( ($p = find_prefixed_value( $x, "MTU:" )) !== false )
            $result["MTU"] = trim($p);
      }

      if ( ($x = find_array_element_text( $txt, "RX packets" )) !== false )
      {
         $x = explode( " ", substr(trim($txt[$x]), 3));
         foreach ( $x as $item )
         {
            $z = explode( ":", $item );
            $result["RX" . ucwords(trim($z[0]))] = $z[1];
         }
      }

      if ( ($x = find_array_element_text( $txt, "TX packets" )) !== false )
      {
         $x = explode( " ", substr(trim($txt[find_array_element_text( $txt, "TX packets" )]), 3));
         foreach ( $x as $item )
         {
            $z = explode( ":", $item );
            $result["TX" . ucwords(trim($z[0]))] = $z[1];
         }
      }

      if ( ($x = find_array_element_text( $txt, "collisions" )) !== false )
      {
         $x = explode( " ", trim($txt[$x]));
         foreach ( $x as $item )
         {
            $z = explode( ":", $item );
            $result[ucwords(trim($z[0]))] = $z[1];
         }
      }

      if ( ($x = find_array_element_text( $txt, "Interrupt" )) !== false )
      {
         $x = trim($txt[$x]);
         $x = explode( " ", str_replace( "Base address", "BaseAddress", $x ));
         foreach ( $x as $item )
         {
            $z = explode( ":", $item );
            $result[trim($z[0])] = $z[1];
         }
      }

      return $result;
   }



   /**
    *
    * ifconfig( device )
    *
    * Returns an array containing the data returned by a call to
    * ifconfig. Returns information for all devices if no device is
    * specified, returns only information for the specified device or
    * false if the device is not listed in /proc/net/dev. 
    *
    * This method returns information that was cached following a call
    * to _ifconfig();
    *
    */
   function ifconfig( $specific_device = false, $bin = "/sbin/ifconfig" )
   {
      static $cache;

      global $network_device_list;
      if ( !is_array($network_device_list) )
         network_device_stats();

      if ( !is_array( $cache ) )
      {
         $cache = array();
         foreach ( $network_device_list as $dev )
            $cache[$dev] = _ifconfig( $dev, $bin );
      }

      if ( $specific_device === false )
         return $cache;

      if ( isset( $cache[$specific_device] ) )
         return $cache[$specific_device];

      return false; // error: device not in /proc/net/dev
   }



