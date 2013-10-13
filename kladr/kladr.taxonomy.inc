<?php

/**
* Set up batch for first and last name loading.
*
* This is where Drupal's Batch API comes into play.
* It's as simple as defining $batch, and then calling batch_set($batch)
*/
function kladr_create() {

  // Update the user profiles to add values to newly added profile fields
  $batch = array(
    'title' => t('Creating kladr vocabulary'), // Title to display while running.
    'operations' => array(), // Operations to complete, in order. Defined below.
    'finished' => '_kladr_vocabulary_finished', // Last function to call.
    'init_message' => t('Initializing...'),
    'progress_message' => t('Fixed @current out of @total.'),
    'error_message' => t('Creating encountered an error.'),
  );

  // Add as many operations as you need. They'll run in the order specified.
  // Parameters can be defined in the (currently) empty arrays and will need
  // to also be added following the $context parameters for the operation
  // functions below.
  $batch['operations'][] = array('_kladr_vocabulary_select', array());
  $batch['operations'][] = array('_kladr_vocabulary_create_structure', array());
  $batch['operations'][] = array('_kladr_vocabulary_insert_data', array());

  // Tip the first domino.
  batch_set($batch);
}


/**
*
*/
function _kladr_vocabulary_select(&$context) {

  $vid = variable_get('kladr_taxonomy', FALSE);
  if($vid) {
    $vocabulary = taxonomy_vocabulary_load($vid);
    
    if ($vocabulary->name) {
      $context['message'] = 'Selected taxonomy vocabulary '. $vocabulary->name;
      $context['finished'] = 1;
      return;
    } 
  }
  $context['finished'] = 0;
}

/**
* Create a taxonomy term and return the tid.
*/
function custom_create_taxonomy_term($name, $vid, $parent_id = 0) {
  $term = new stdClass();
  $term->name = $name;
  $term->vid = $vid;
  $term->parent = array($parent_id);
  taxonomy_term_save($term);
  return $term->tid;
}

function get_taxonomy_tid($name, $vid, $parent_tid = 0) {
  $query = db_select('taxonomy_term_data', 't');
  $query->join('taxonomy_term_hierarchy', 'h', 'h.tid = t.tid');
  $query->addField('t', 'tid');
  $query->condition('h.parent', $parent_tid);
  $query->condition('t.name', trim($name));
  $query->condition('t.vid', $vid);
  $tids = $query->execute()->fetchCol();

  return isset($tids) && is_array($tids) && count($tids) > 0 ? $tids[0] : NULL;
}

function getParentTaxonomyId($cc, $rrr) {
  $sql = "SELECT tid FROM {kladr_taxonomy} WHERE cc = :cc and rrr = :rrr";
  $params = array(':cc' => $cc, ':rrr' => $rrr);
  return db_query($sql, $params)->fetchField();
}

/**
*
*/

function _kladr_vocabulary_create_structure(&$context) {

  $vid = variable_get('kladr_taxonomy', FALSE);
  $start_from_row = variable_get('kladr_taxonomy_last', 1);

  if(!$vid OR $start_from_row > 1) {
      $context['finished'] = 1;
      return;
  }

  // open in read-only mode
  $db = dbase_open(drupal_get_path('module', 'kladr') .'/BASE/KLADR.DBF', 0);
  if ($db) {
    $record_numbers = dbase_numrecords($db);
    for ($i = 1; $i <= $record_numbers; $i++) {
      $row= dbase_get_record($db, $i);
      
      $name=trim(mb_convert_encoding($row[0], "UTF-8", "cp866")); // Наименование
      $socr=trim(mb_convert_encoding($row[1], "UTF-8", "cp866")); // Сокращенное наименование типа объекта
      $code=$row[2]; 		// Код
      $index=$row[3]; 	// Почтовый индекс
      $gninmb=$row[4]; 	// Код ИФНС
      $uno=$row[5]; 		// Код территориального участка ИФНС
      $ocatd=$row[6]; 	// Код ОКАТО
      $status=$row[7]; 	// Статус объекта
      
      $CC = (int)substr($code, 0, 2);
      $RRR = (int)substr($code, 2, 3);
      $GGG = (int)substr($code, 5, 3);
      $PPP = (int)substr($code, 8, 3);
      $AA = (int)substr($code, 11, 2);

      if ($RRR==0 AND $GGG==0 AND $PPP==0 AND $AA==0){ // region
        $name = $name . ' ' . $socr;
        $tid = get_taxonomy_tid($name, $vid, 0);
        if ($tid) {
          drupal_set_message('* region ' . $name . ' already exists (tid: ' . $tid . ')');
        } else {
          $tid = custom_create_taxonomy_term($name, $vid, 0);
          drupal_set_message('+ added region: ' . $name);
        }

        if (!getParentTaxonomyId($CC, 0))
          db_query('INSERT INTO {kladr_taxonomy} (cc, rrr, tid, name) VALUES(:cc, 0, :tid, :name)', array(':cc' => $CC, ':tid' => $tid, ':name' => $name));

      } else if ($GGG==0 AND $PPP==0 AND $AA==0) { // subregion
        $name = $name . ' ' . $socr;
        $parent_tid = getParentTaxonomyId($CC, 0);
        if ($parent_tid) {
          $tid = get_taxonomy_tid($name, $vid, $parent_tid);
          // drupal_set_message($name . ' -> parent tid: ' . $parent_tid . ', tid: ' . $tid);
          if ($tid) {
            drupal_set_message(' * subregion: ' . $name . ' already exists in taxonomy (tid: ' . $tid . ')');
          } else {
            $tid = custom_create_taxonomy_term($name, $vid, $parent_tid);
            drupal_set_message(' + added subregion: ' . $name . ', tid: ' . $tid);
          }

          if (!getParentTaxonomyId($CC, $RRR))
            db_query('INSERT INTO {kladr_taxonomy} (cc, rrr, tid, name) VALUES(:cc, :rrr, :tid, :name)', array(':cc' => $CC, ':rrr' => $RRR, ':tid' => $tid, ':name' => $name));
        } else {
          drupal_set_message(' - parent not found for subregion: ' . $name . ", id: " . $CC . ", sub_id: " . $RRR . ', parent_tid: ' . $parent_tid, 'warning');
        }
      }
    }
  }
  dbase_close($db);

  $context['finished'] = 1;
}

function startsWith($haystack, $needle)
{
  return !strncmp($haystack, $needle, strlen($needle));
}

/**
*
*/
function _kladr_vocabulary_insert_data(&$context) {

  $vid = variable_get('kladr_taxonomy', FALSE);
  if(!$vid) {
      $context['finished'] = 1;
      return;
  }

  // open in read-only mode
  $db = dbase_open(drupal_get_path('module', 'kladr') .'/BASE/KLADR.DBF', 0);

  if ($db) {
    $insert_rows_for_one_time = 50000;
    $start_from_row = variable_get('kladr_taxonomy_last', 1);
    
    $record_numbers = dbase_numrecords($db);
    
    drupal_set_message('DB opened, ' . $record_numbers . ' records');
    for ($i = $start_from_row; 
         $i <= $record_numbers AND $i <= ($start_from_row + $insert_rows_for_one_time); 
         $i++) {
      variable_set('kladr_taxonomy_last', $i);
      
      $row= dbase_get_record($db, $i);
      
      $name=trim(mb_convert_encoding($row[0], "UTF-8", "cp866")); // Наименование
      $socr=trim(mb_convert_encoding($row[1], "UTF-8", "cp866")); // Сокращенное наименование типа объекта
      $code=$row[2]; 		// Код
      $index=$row[3]; 	// Почтовый индекс
      $gninmb=$row[4]; 	// Код ИФНС
      $uno=$row[5]; 		// Код территориального участка ИФНС
      $ocatd=$row[6]; 	// Код ОКАТО
      $status=$row[7]; 	// Статус объекта
      
      $CC = (int)substr($code, 0, 2);
      $RRR = (int)substr($code, 2, 3);
      $GGG = (int)substr($code, 5, 3);
      $PPP = (int)substr($code, 8, 3);
      $AA = (int)substr($code, 11, 2);
      
      // drupal_set_message("::1 - term: name=$name, $code=$CC|$RRR");

      if ($RRR==0 AND $GGG==0 AND $PPP==0 AND $AA==0){
        // debug stub
        // we process this case in _kladr_vocabulary_create_structure
      } else if ($GGG==0 AND $PPP==0 AND $AA==0){
        // debug stub
        // we process this case in _kladr_vocabulary_create_structure
      } else if (($status>0 OR $socr='г') AND $AA==0 AND !startsWith($name, 'сдт')){
        $parent_tid = getParentTaxonomyId($CC, $RRR);
        if ($parent_tid) {
          $tid = get_taxonomy_tid($name, $vid, $parent_tid);
          if (!$tid)
            $tid = custom_create_taxonomy_term($name, $vid, $parent_tid);
          // drupal_set_message(' + added settlement: ' . $name);
        } else {
          drupal_set_message(' - parent not fount for settlement: ' . $name . ", cc: " . $CC . ", rrr: " . $RRR);
        }
      }
      
    }
    drupal_set_message('DB last inserted record '. $i);
    
    dbase_close($db);
  }
  $context['finished'] = 1;
}

/**
*
*/
function _kladr_vocabulary_finished($success, $results, $operations) {

  if ($success) {
    $message = t('KLADR taxonomy vocabulary updated.');
  }
  else {
    $message = t('Finished with error.');
  }
  drupal_set_message($message);
}


/**
 * Save recursive array of terms for a vocabulary.
 *
 * Example:
 * <code><?php
 * $terms = array(
 *   'Species' => array(
 *     'Dog',
 *     'Cat',
 *     'Bird' ),
 *   'Sex' => array(
 *     'Male',
 *     'Female' ) )
 * _save_terms_recursive( $vid, $terms );
 * </code>
 *
 * @param int $vid Vocabulary id
 * @param array $terms Recursive array of terms
 * @param int $ptid Parent term id (generated by taxonomy_save_term) 
 */
function _save_terms_recursive( $vid, &$terms, $ptid=0 ) {
  foreach ( $terms as $k => $v ) {
    // simple check for numeric indices (term array without children)
    $name = is_string( $k ) ? $k : $v;
    $term = array( 'vid' => $vid, 'name' => $name, 'parent' => $ptid );
    drupal_set_message("_save_terms_recursive - term: " . print_r($term, true));
    // taxonomy_save_term( $term );
    if ( is_array( $v ) && count( $v ) )
      _save_terms_recursive( $vid, $terms[ $k ], $term[ 'tid' ] );
  }
}
