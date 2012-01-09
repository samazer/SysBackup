<?php // -*- mode: php; mode: mmm; coding: utf-8-unix; -*-
/** 
 *
 * Virtual Backup Filesystem hashlist code
 *
 * The hashlists turned out to be far more significant than expected
 * in terms of code - so factoring them out as their own objects
 * reduces the code complexity and increases readability quite a bit
 * (in principal - hopefully there will be time to think while the
 * code is being written so that it reflects the basic truth.)
 *
 * <p><b>Copyright</b>
 *
 * <p>Copyright (c) 2011 by Sam Azer, All Rights Reserved.
 *
 * <p>This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
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
 * @package sysbackup
 * @subpackage sysbackup
 * @license http://www.gnu.org/licenses/agpl.html AGPL
 * @copyright 2011 by Sam Azer, All rights reserved
 * @link http://www.azertech.net/
 * @link http://www.samazer.net/about
 *
 */



/**
 * 
 * stats collected during the backup operation
 *
 */
class backupvfs_stats_c
{
   var $start;               // microtime() at start
   var $tarball_count;       // total number of tarballs
   var $tarball_size;        // total size of the tarballs
   var $hardlinked_count;    // total number that were hardlinked
   var $hardlinked_size;     // total size of the hardlinked tarballs


   function reset()
   {
      $this->start            = microtime(true);

      $this->tarball_count    = 0;
      $this->tarball_size     = 0;

      $this->hardlinked_count = 0;
      $this->hardlinked_size  = 0;
   }


   function __construct()
   {
      $this->reset();
      $this->start            = NULL;
   }


   function get_stats( $prefix = "" )
   {
      return array( "{$prefix}start"               => $this->start,
                    "{$prefix}tarball_count"       => $this->tarball_count,
                    "{$prefix}tarball_size"        => $this->tarball_size,
                    "{$prefix}hardlinked_count"    => $this->hardlinked_count,
                    "{$prefix}hardlinked_size"     => $this->hardlinked_size,
                   );
   }


   function add_tarball( $size )
   {
      $this->tarball_count++;
      $this->tarball_size += $size;
   }


   function add_hardlinked( $size )
   {
      $this->hardlinked_count++;
      $this->hardlinked_size += $size;
   }
}


/**
 *
 * Hashlist class
 *
 * The hashlist code in the base IO class is getting really
 * complicated as I try to incorporate the code to properly handle
 * rotations. The solution is to factor the hashlists out of the
 * baseIO code so that it can be possible to have many hashlists in an
 * object - while still allowing the code to be readable. Here's my
 * first try:
 *
 * Basically a backupset consists of a set of tarballs (along with a
 * few compressed files; but we'll call them all tarballs.) Each
 * tarball has a size and a basic SHA1 hash. Later on, after
 * encryption, the tarball has another hash for the encrypted version
 * of the file but we're not dealing with that yet.
 *
 * The problem is that we don't want to add the tarball to the
 * backupset if there is another tarball, either in the current
 * backupset or in the previous backupset, that already has the same
 * basic SHA1 hash. To be able to do this we need to be able to find
 * the previous backupset for any given rotation (daily, weekly or
 * monthly,) and check the list of tarball hash values. There are some
 * complications that also need to be managed, ie: if the daily and
 * weekly or daily and monthly rotations are on the same partition we
 * need to check the hash lists for the previous backupsets in the
 * daily rotation instead of the previous backupset in the weekly or
 * monthly rotation.
 *
 * In any case, as you can see, we want to keep track of two different
 * hashlists as we are performing the backup: The hashlist for the
 * current backupset and the hashlist for the previous backupset.
 *
 * While we're doing this we will also need some stats: total number
 * of tarballs, total size of the backupset, total number of tarballs
 * that have been hardlinked and total size of the hardlinked
 * tarballs.
 *
 */
class backupvfs_hashlist_c
{
   var $app;          // a reference to the associated application object
   var $vfs;          // a reference to the associated Virtual File System object

   var $hashlist;     // array of hash values by relative file name
   var $hashlist_rev; // array of relative file names by hash value
   var $sizelist;     // array of file sizes by relative file name

   var $hardlinked;   // array of hash values by relative file name for hardlinked files only

   var $stats;        // local stats
   var $job_stats;
   var $step_stats;


   function current_reset() { $this->stats->reset();      }
   function job_reset()     { $this->job_stats->reset();  }
   function step_reset()    { $this->step_stats->reset(); }

   function all_reset()
   {
      $this->stats->reset();
      $this->job_stats->reset();
      $this->step_stats->reset();
   }

   function file_exists( $rel_ref )
   {
      return isset( $this->hashlist[$rel_ref] );
   }

   function hash_exists( $sha1 )
   {
      return isset($this->hashlist_rev[$sha1]);
   }

   function filesize($rel_ref)      { return isset($this->sizelist[$rel_ref])
                                           ? $this->sizelist[$rel_ref]   : false; }
   function filehash($rel_ref)      { return isset($this->hashlist[$rel_ref])
                                           ? $this->hashlist[$rel_ref]   : false; }
   function fileref($hash)          { return isset($this->hashlist_rev[$hash])
                                           ? $this->hashlist_rev[$hash]  : false; }

   function is_hardlinked($rel_ref) { return isset($this->hardlinked[$rel_ref])
                                           ? $this->hardlinked[$rel_ref] : false; }

   function get_stats()          
   { 
      return $this->stats->get_stats( "current_" )
           + $this->job_stats->get_stats( "job_" )
           + $this->step_stats->get_stats( "step_" )
             ;
   }


   function __construct( &$vfs_object )
   {
      $this->vfs        = &$vfs_object;
      $this->app        = &$vfs_object->app;

      $this->stats      = new backupvfs_stats_c();
      $this->job_stats  = new backupvfs_stats_c();
      $this->step_stats = new backupvfs_stats_c();

      $this->hashlist         = array();
      $this->hashlist_rev     = array();
      $this->sizelist         = array();
      $this->hardlinked       = array();

      $this->all_reset();
   }


   function register( $rel_fn, $sha1, $size, $hardlinked_flag )
   {
      $this->hashlist[$rel_fn]   = $sha1;   // array of hash values by relative file name
      $this->sizelist[$rel_fn]   = $size;   // array of file sizes by relative file name

      // only add once to the hashlist_rev array
      if ( !isset( $this->hashlist_rev[$sha1] ) )
         $this->hashlist_rev[$sha1] = $rel_fn; // array of relative file names by hash value

      $this->stats->add_tarball( $size );
      $this->job_stats->add_tarball( $size );
      $this->step_stats->add_tarball( $size );

      if ( $hardlinked_flag )
      {
         $this->hardlinked[$rel_fn] = $sha1;
         $this->stats->add_hardlinked( $size );
         $this->job_stats->add_hardlinked( $size );
         $this->step_stats->add_hardlinked( $size );
      }
   }


  /**
   *
   * write a hashlist to a file
   *
   */
   function _write( $hashfn, $hashlist )
   {
      $txt = "";
      foreach ( $hashlist as $fn => $sha1 )
         $txt = $txt
              . str_replace( "=", "%3d", $fn )
              . "="
              . $sha1
              . "\n"
              ;

      if ( $this->vfs->_vfs_file_put_contents( $hashfn, $txt ) === false )
         $this->vfs->fatal( "unable to write to {$hashfn}" );

      if ( $this->vfs->_vfs_chmod( $hashfn, $this->vfs->get_umaskfiles() ) === false )
         $this->vfs->fatal( "unable to set permissions on {$hashfn}" );

      return false;
   }


  /**
   *
   * read a hashlist from a file
   *
   */
   function _read( $fn )
   {
      if ( !$this->vfs->_vfs_file_exists( $fn ) )
         $this->vfs->fatal( "unable to find {$fn}" );

      if ( ($text = $this->vfs->_vfs_file_get_contents( $fn )) === false )
         $this->vfs->fatal( "unable to read {$fn}" );

      $text = explode( "\n", $text );

      $hashlist = array();
      foreach ( $text as $x )
      {
         if ( !$x )
            continue;

         $x = explode( "=", $x );
         $hashlist[str_replace( "%3d", "=", $x[0] )] = $x[1];
      }

      return $hashlist;
   }


  /**
   *
   * reverse a hashlist
   *
   */
   function _reverse( $hashlist )
   {
      $hashlist_rev = array();
      foreach ( $hashlist as $fn => $sha1 )
      {
        /**
         * more than one file might have the same hash but we only
         * care about one of them (any one) because we assume that
         * hardlinks will always work fine in the backupset folder
         * (It's the same partition for all files in that folder.)
         *
         * NOTE that this can fail miserably if rotations are saved to
         * a different partition and the system writes a symlink to a
         * previous backupset. This is an error. It's better to turn
         * off the linking feature completely in the case where
         * multiple partitions are being used.
         */
         if ( isset($hashlist_rev[$sha1]) )
            continue;

         $hashlist_rev[$sha1] = $fn;
      }

      return $hashlist_rev;
   }


  /**
   *
   * Read a hashlist
   *
   */
   function read( $hashfile_name )
   {
      $this->hashlist     = $this->_read( $hashfile_name );
      $this->hashlist_rev = $this->_reverse( $this->hashlist );
   }



   function save_all()
   {
      $hash_fn = $this->app->get_hostname() . EXT_HASHLIST;
      $hard_fn = $this->app->get_hostname() . EXT_HARDLINKED;

      if ( $this->app->verbose() )
         $this->tee( "writing hashlist {$hash_fn}" );

      $this->_write( dir_fn( $this->vfs->get_path_backupset(), $hash_fn ), $this->hashlist   );

      if ( $this->app->verbose() )
         $this->tee( "writing hardlinked file list {$hard_fn}" );

      $this->_write( dir_fn( $this->vfs->get_path_backupset(), $hard_fn ), $this->hardlinked );

      // always returns no error result
      return false; 
   }
}



