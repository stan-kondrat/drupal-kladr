<?php
// $Id$

/**
* @file
* Contains forms and form handler functions that make up the batch demo administration section.
*/

function kladr_settings() {


  $options = array();  
  $vocabularies = taxonomy_get_vocabularies();
  foreach ($vocabularies as $vocabulary) {
    $options[$vocabulary->vid] = $vocabulary->name;
  }
  
  $form = array();
  $form['kladr_taxonomy'] = array(
    '#type' => 'select',
    '#title' => t('KLADR taxonomy vocabulary'),
    '#default_value' => variable_get('kladr_taxonomy', FALSE),
    '#options' => $options,
    '#description' => t('The timer.'),
  );

  return system_settings_form($form);
}


/**
* Use Drupal's Form API (FAPI) to wire up buttons to kick off batch operations.
*/
function kladr_form($form, &$form_state) {

  $form['kladr_batches'] = array(
   '#type' => 'fieldset',
   '#title' => 'KLADR',
   '#description' => 'Run batch operations for KLADR. ' . l(t('KLADR taxonomy vocabulary'), 'admin/content/taxonomy/'.variable_get('kladr_taxonomy', FALSE)),
   '#collapsible' => TRUE,
   '#collapsed' => FALSE,
  );
 
  $form['kladr_batches']['new_kladr_vocabulary'] = array(
    '#type' => 'submit',
    '#value' => t('Load KLADR to taxonomy vocabulary'),
  );
  $form['kladr_batches']['sync_vocabulary'] = array(
    '#type' => 'submit',
    '#value' => "Начать загрузку сначала",
  );
 
  return $form;
}

/**
* Submit handler for kladr_form();
*/
function kladr_form_submit($form, &$form_state) {

  // drupal_set_message('За раз добавляется 25 наименований, послед. form: <pre>' . print_r($form_state, true) . '</pre>');
  require_once(drupal_get_path('module', 'kladr') .'/kladr.taxonomy.inc');

  switch ($form_state['values']['op']) {

    case t("Load KLADR to taxonomy vocabulary"):
      kladr_create();
      break;

    case "Начать загрузку сначала":
      variable_set('kladr_taxonomy_last', 1);
      db_query('TRUNCATE TABLE {kladr_taxonomy}');
      drupal_set_message('За раз добавляется 25 наименований, послед. ' . variable_get('kladr_taxonomy_last', 1));
      break;

    // Add more cases to trigger batch operations for more buttons.

  }
}
