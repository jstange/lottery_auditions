<?php

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
  $date1 = "";
  if(is_object($datefield['value'])){
    $date1 = strtotime($datefield['value']);
  } elseif(is_array($datefield['value'])){
    $date1 = strtotime($datefield['value']['date']);
  } else {
    $date1 = strtotime($datefield['value']);
  }

  if(is_object($datefield['value2'])){
    $datefield['value2']->setDate(date('Y', $date1), date('m', $date1), date('d', $date1));
  } elseif(is_array($datefield['value2'])){
    $datefield['value2']['date'] = $datefield['value']['date'];
  } else {
    $date2 = strtotime($datefield['value2']);
    $datefield['value2'] = date('Y-m-d', $date1)." ".date('H:i:s', $date2);
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
/*
function auditioneer_form_submit($form, &$form_state){
}
*/

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

    $start = strtotime($form_state['values']['field_auditioneer_time_blocks'][LANGUAGE_NONE][$count]['field_auditioneer_timerange'][LANGUAGE_NONE][0]['value']);
    $end = strtotime($form_state['values']['field_auditioneer_time_blocks'][LANGUAGE_NONE][$count]['field_auditioneer_timerange'][LANGUAGE_NONE][0]['value2']);

    if(($end - $start)/3600 > 12){
      form_set_error($form_elt, "Time blocks cannot span more than 12 hours");
    } elseif(($end - $start) < 900) {
      form_set_error($form_elt, "Time blocks must be 15 minutes or longer");
    }
    $subcount = 0;
    while(array_key_exists($subcount, $form['field_auditioneer_time_blocks'][LANGUAGE_NONE])){
      if($count == $subcount){
	$subcount++;
	continue;
      }
      $substart = strtotime($form_state['values']['field_auditioneer_time_blocks'][LANGUAGE_NONE][$subcount]['field_auditioneer_timerange'][LANGUAGE_NONE][0]['value']);
      $subend = strtotime($form_state['values']['field_auditioneer_time_blocks'][LANGUAGE_NONE][$subcount]['field_auditioneer_timerange'][LANGUAGE_NONE][0]['value2']);
      $form_subelt = "field_auditioneer_time_blocks][".LANGUAGE_NONE."][$subcount][field_auditioneer_timerange][und";
      if (($substart >= $start && $substart <= $end) ||
	  ($subend >= $start && $subend <= $end)){
	form_set_error($form_subelt, "Audition time block ".date('Y-m-d H:i:s', $substart)." - ".date('Y-m-d H:i:s', $subend)." overlaps with another block");
      }
      $subcount++;
    }

    $count++;
  }
}
