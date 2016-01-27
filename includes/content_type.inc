<?php

/*
 * Take a FAPI date field and return DateTime object(s) based on it, with
 * attendant timezone weirdness taken care of.
 */
function _auditioneer_get_datetime_from_datefield_array($datefield){
  $stamp_1 = strtotime($datefield['value']);
  if(array_key_exists('offset', $datefield)){
    $stamp_1 += $datefield['offset'];
  }
  $date1 = new DateTime();
  $date1->setTimeStamp($stamp_1);

  $date_2 = null;
  if(array_key_exists('value2', $datefield)){
    $stamp_2 = strtotime($datefield['value2']);
    if(array_key_exists('offset2', $datefield)){
      $stamp_2 += $datefield['offset2'];
    }
    $date2 = new DateTime();
    $date2->setTimeStamp($stamp_2);
  }
  return array($date1, $date2);
}

/**
 * Make sure the ends of our audition time blocks are on the same days as their
 * respective beginnings (really we're just collecting the time bit).
 * 
 * It seems we have to account for Date fields being stored in any number of
 * formats (flat strftime()-style strings, arrays of strings representing
 * date and time separately, or DateTime objects). Whoever wrote this crap is
 * the first one up against the wall when the revolution comes.
 */
function _auditioneer_datefield_sync_dates(&$datefield){
  $date1 = new DateTime();
  if($datefield['value'] instanceof DateTime){
    $date1 = $datefield['value'];
  } elseif(is_array($datefield['value'])){
    $date1->setTimeStamp(strtotime($datefield['value']['date']));
  } else {
    $date1->setTimeStamp(strtotime($datefield['value']));
  }

  if($datefield['value2'] instanceof DateTime){
    $datefield['value2']->setDate($date1->format('Y'), $date1->format('m'), $date1->format('d'));
  } elseif(is_array($datefield['value2'])){
    $datefield['value2']['date'] = $date1->format('Y-m-d');
  } else {
    $date2 = new DateTime();
    $date2->setTimeStamp(strtotime($datefield['value2']));
    $datefield['value2'] = $date1->format('Y-m-d')." ".$date2->format('H:i:s');
  }
}


function auditioneer_field_auditioneer_time_blocks_validate($element, &$form_state, $form) {    
  $count = 0;
  drupal_add_css(drupal_get_path('module', 'auditioneer') . '/auditioneer.css', array('group' => CSS_DEFAULT, 'type' => 'file'));
  while(array_key_exists($count, $form['field_auditioneer_time_blocks'][LANGUAGE_NONE])){
    // XXX This is the wrong way to do this, but form_set_value just doesn't
    // fucking work
    _auditioneer_datefield_sync_dates($form_state['values']['field_auditioneer_time_blocks'][LANGUAGE_NONE][$count]['field_auditioneer_timerange'][LANGUAGE_NONE][0]);
    _auditioneer_datefield_sync_dates($form_state['input']['field_auditioneer_time_blocks'][LANGUAGE_NONE][$count]['field_auditioneer_timerange'][LANGUAGE_NONE][0]);

    if(array_key_exists("entity", $form_state['values']['field_auditioneer_time_blocks'][LANGUAGE_NONE][$count])){
      _auditioneer_datefield_sync_dates($form_state['values']['field_auditioneer_time_blocks'][LANGUAGE_NONE][$count]['entity']->field_auditioneer_timerange[LANGUAGE_NONE][0]);
    }


    $count++;
  }

  // Now it should be safe to invoke date_combo_validate
  _auditioneer_datefield_sync_dates($element['field_auditioneer_timerange'][LANGUAGE_NONE][0]['#value']);
  $element['field_auditioneer_timerange'][LANGUAGE_NONE][0]['value2']['#value']['date'] = $element['field_auditioneer_timerange'][LANGUAGE_NONE][0]['value']['#value']['date'];
  $element['field_auditioneer_timerange'][LANGUAGE_NONE][0]['value2']['date']['#value'] = $element['field_auditioneer_timerange'][LANGUAGE_NONE][0]['value']['date']['#value'];
  date_combo_validate($element['field_auditioneer_timerange'][LANGUAGE_NONE][0], $form_state);

}


/**
 * Implements hook_form_alter().
 */
function auditioneer_form_alter(&$form, $form_state, $form_id) {
  _auditioneer_display_bits();
  if($form_id == "auditioneer_audition_node_form"){

    $finalized = false;
    if($form_state['node']->nid){
      $audition = node_load($form_state['node']->nid);
      if($audition->field_auditioneer_group_interval[LANGUAGE_NONE][0]['value'] > 0){
        $finalized = true;
      }
    }

    drupal_add_css(drupal_get_path('module', 'auditioneer') . '/auditioneer.css', array('group' => CSS_DEFAULT, 'type' => 'file'));
    drupal_add_js(drupal_get_path('module', 'auditioneer') . '/auditioneer.js');
    array_unshift($form['#validate'], 'auditioneer_form_validate');
    $form['wrapper'] = array(
      '#prefix' => '<div id="audition_form_wrapper">',
      '#suffix' => '</div>'
    );
    $form['field_auditioneer_time_blocks']['wrapper'] = array(
      '#prefix' => '<div id="audition_time_block">',
      '#suffix' => '</div>',
    );
    $count = 0;
    unset($form['field_auditioneer_finalized']); // internal-use field, hide from users
    $form['field_auditioneer_location'][LANGUAGE_NONE][0]['country']['#value'] = "US"; // XXX why doesn't #default_value work?
    $form['field_auditioneer_location'][LANGUAGE_NONE][0]['country']['#weight'] = 500;
    $form['options']['status']['#default_value'] = 0;
    $form['options']['promote']['#default_value'] = 0;
    $form['#prefix'] = "<h1>This audition's assignments have been finalized. Only some fields are available for editing.</h1>";
    unset($form['actions']['preview']);
    if($finalized){
      foreach (array('field_auditioneer_collect', 'field_auditioneer_deadline', 'field_auditioneer_group_interval', 'field_auditioneer_time_blocks') as $fieldname){
        $form[$fieldname]['#access'] = false;
      }
    }

    while(array_key_exists($count, $form['field_auditioneer_time_blocks'][LANGUAGE_NONE])){

      //      $form['field_auditioneer_time_blocks'][LANGUAGE_NONE][$count]['field_auditioneer_timerange'][LANGUAGE_NONE][0]['#ajax'] = array(
      //	'event' => 'change',
      //        'callback' => '_auditioneer_timeblock_ajax',
      //        'wrapper' => 'audition_block_timerange_'.$count
      //      );

      $count++;
    }
  }
}


function _auditioneer_timeblock_ajax(&$form, $form_state){

  $commands = array();
  $count = 0;
  while(array_key_exists($count, $form['field_auditioneer_time_blocks'][LANGUAGE_NONE])){
  }


  return($form['field_auditioneer_time_blocks']['wrapper']);
}

/**
 */
function auditioneer_form_submit($form, &$form_state){
  variable_set('menu_rebuild_needed', TRUE);
}

/**
 * Implements validation from the Form API.
 * 
 * @param $form
 *   A structured array containing the elements and properties of the form.
 * @param $form_state
 *   An array that stores information about the form's current state 
 *   during processing.
 */
function auditioneer_form_validate($form, &$form_state){
  $count = 0;

  while(array_key_exists($count, $form['field_auditioneer_time_blocks'][LANGUAGE_NONE])){
    $css_id = "#audition_block_timerange_$count";
    if(!isset($form['field_auditioneer_time_blocks'][LANGUAGE_NONE][$count]['field_auditioneer_timerange'][LANGUAGE_NONE][0]['#value'])){
      break;
    }
    $form_elt = "field_auditioneer_time_blocks][".LANGUAGE_NONE."][$count][field_auditioneer_timerange][und";
    list($start, $end) = _auditioneer_get_datetime_from_datefield_array($form_state['values']['field_auditioneer_time_blocks'][LANGUAGE_NONE][$count]['field_auditioneer_timerange'][LANGUAGE_NONE][0]);

    $start_stamp = $start->getTimestamp();
    $end_stamp = $end->getTimestamp();

    if(($end_stamp - $start_stamp)/3600 > 12){
      form_set_error($form_elt, "Time blocks cannot span more than 12 hours");
    } elseif(($end_stamp - $start_stamp) < 900) {
      form_set_error($form_elt, "Time blocks must be 15 minutes or longer");
    }
    $subcount = 0;
    while(array_key_exists($subcount, $form['field_auditioneer_time_blocks'][LANGUAGE_NONE])){
      if($count == $subcount){
        $subcount++;
        continue;
      }
      list($substart, $subend) = _auditioneer_get_datetime_from_datefield_array($form_state['values']['field_auditioneer_time_blocks'][LANGUAGE_NONE][$subcount]['field_auditioneer_timerange'][LANGUAGE_NONE][0]);
      $form_subelt = "field_auditioneer_time_blocks][".LANGUAGE_NONE."][$subcount][field_auditioneer_timerange][und";
      if (($substart->getTimestamp() > $start_stamp && $substart->getTimestamp() < $end_stamp) ||
        ($subend->getTimestamp() > $start_stamp && $subend->getTimestamp() < $end_stamp)){
        form_set_error($form_subelt, "Audition time block ".$substart->format('l F jS g:ia')." - ".$subend->format('g:ia')." overlaps with another block.");
      }
      $subcount++;
    }

    $count++;
  }
}
