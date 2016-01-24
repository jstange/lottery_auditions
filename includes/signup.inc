<?php

/**
 * Fetch the start and end time for a audition. Useful for sorting displays,
 * hiding old stuff, etc.
 */

$_auditioneer_cached_times = array();
function _auditioneer_get_times($audition){
  global $_auditioneer_cached_times;
  if(is_array($_auditioneer_cached_times) &&
    array_key_exists($audition->nid, $_auditioneer_cached_times)){
    return $_auditioneer_cached_times[$audition->nid];
  }
  $timeblocks = field_view_field('node', $audition, 'field_auditioneer_time_blocks');
  $count = $start = $end = 0;
  while(array_key_exists($count, $timeblocks)){
    $realitem = array_pop($timeblocks[$count]['entity']['field_collection_item']);
    $day = field_view_field('field_collection_item', $realitem['field_auditioneer_timerange']['#object'], 'field_auditioneer_timerange', 'full');
    $time = strtotime(preg_replace('/ to .*/', "", strip_tags($day[0]['#markup'])));
    if($start == 0 || $time < $start){
      $start = $time;
    }
    if($end == 0 || $time > $end){
      $end = $time;
    }
    $count++;
  }
  $_auditioneer_cached_times[$audition->nid] = array($start, $end);
  return array($start, $end);
}

function _auditioneer_sort_by_date($a, $b){
  list($a_start, $a_end) = _auditioneer_get_times($a);
  list($b_start, $b_end) = _auditioneer_get_times($b);

  return ($a_start < $b_start) ? 1 : -1;
}

function _auditioneer_timeblock_sort($a, $b){
  $a_start = strtotime($a['field_auditioneer_timerange']['#items'][0]['value']);
  $b_start = strtotime($b['field_auditioneer_timerange']['#items'][0]['value']);

  return ($a_start > $b_start) ? 1 : -1;
}

function auditioneer_signup_page($nid = null){
  _auditioneer_display_bits();
  if($nid){
    $audition = node_load($nid);
    $formbits = drupal_get_form('audition_signup_'.$nid, $nid, $audition);
    return($formbits);
  } else {
    $auditions = node_load_multiple(array(), array('type' => "auditioneer_audition"));
    uasort($auditions, '_auditioneer_sort_by_date');

    $output = array();
    $output['#markup'] = "";
    foreach ($auditions as $nid => $audition){
      // Skip auditions that are fully in the past or aren't yet published.
      list($start, $end) = _auditioneer_get_times($audition);
      if($end < time() || $audition->status != NODE_PUBLISHED){
        continue;
      }
      $output['#markup'] .= drupal_render(drupal_get_form('audition_signup_'.$nid, $nid, $audition));
    }
    return $output;
  }

}


function auditioneer_entity_info_alter(&$entity_info) {
  _auditioneer_display_bits();
  $entity_info['node']['view modes']['auditioneer_audition_signup_form'] = array(
    'label' => t('Audition Signup Form'),
    'custom settings' => TRUE,
  );
  $entity_info['field_collection_item']['view modes']['auditioneer_audition_signup_form'] = array(
    'label' => t('Audition Signup Form'),
    'custom settings' => TRUE,
  );
}


function _auditioneer_signup_form($form, &$form_state, $args){
  global $user;
  $nid = $args[0];
  $audition = $args[1];
  $disabled = false;
  $ext_requests = db_select('auditioneer_signups', 's')
    ->fields('s', array('availability', 'placement', 'slot_type'))
    ->condition('nid', $nid)
    ->condition('uid', $user->uid)
    ->execute()
    ->fetchAssoc();
  $availability = $slot_type = null;
  $submit_text = t('Sign Up');
  if(count($ext_requests) > 0 && is_array($ext_requests) && array_key_exists('availability', $ext_requests)){
    $availability = unserialize($ext_requests['availability']);
    $slot_type = $ext_requests['slot_type'];
    if(count($availability)  > 0){
      $submit_text = t('Update My Availability');
    }
  }
  $global_contact_fields = Array();
  $contact_items = field_view_field('node', $audition, 'field_auditioneer_collect');
  foreach ($contact_items['#items'] as $delta => $value){
    $global_contact_fields[] = $value['value'];
  }

  $form['contact_email'] = array(
    '#type' => 'textfield',
    '#size' => 35,
    '#title' => t('Email'),
    '#required' => in_array("Email", $global_contact_fields)
  );
  $form['contact_name'] = array(
    '#type' => 'textfield',
    '#size' => 35,
    '#title' => t('Name'),
    '#required' => in_array("Name", $global_contact_fields)
  );
  $form['contact_phone'] = array(
    '#type' => 'textfield',
    '#size' => 16,
    '#title' => t('Phone'),
    '#required' => in_array("Phone", $global_contact_fields)
  );
  $form['contact_resume'] = array(
    '#type' => 'file',
    '#title' => t('Photo/Resume'),
    '#required' => in_array("Photo/Resume", $global_contact_fields)
  );

  $timeblocks = field_view_field('node', $audition, 'field_auditioneer_time_blocks');
  $address = "<div class='auditioneer_audition_address'>".drupal_render(field_view_field('node', $audition, 'field_auditioneer_location'))."</div>";
  $instructions = "<div class='auditioneer_audition_instructions'>".drupal_render(field_view_field('node', $audition, 'field_auditioneer_instructions'))."</div>";
  $deadline = drupal_render(field_view_field('node', $audition, 'field_auditioneer_deadline'));
  $deadline_str = field_view_field('node', $audition, 'field_auditioneer_deadline');
  $deadline_time = strtotime($deadline_str['#items'][0]['value']);
  $banner_text = "";
  if(time() >= $deadline_time){
    $disabled = true;
  }
  $form['audition_signup_fieldset'] = array(
    '#prefix' => "<h2>".t($audition->title)."</h2>",
    '#type' => 'fieldset',
    '#title' => t($audition->title),
    '#title_display' => 'before',
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#attributes' => array('class' => array('auditioneer_audition_signup_wrapper')),
    'audition_id' => array(
      '#type' => 'hidden',
      '#value' => $nid
    ),
    'info_panel' => array(
      '#type' => 'fieldset',
      '#collapsed' => FALSE,
      '#collapsible' => FALSE,
      '#attributes' => array('class' => array('auditioneer_audition_signup_dummy_panel')),
      '#suffix' => "<div class='auditioneer_audition_infoblock'>".$address.$instructions."</div>"
    )
  );
  $form['#attributes'] = array('class' => array('auditioneer_audition_signup'));
  $form['#submit'] = array('auditioneer_audition_signup_submit_callback');
  $form['#validate'] = array('auditioneer_audition_signup_validate_callback');
  //  list($start, $end) = _auditioneer_get_times($audition);
  $banner = "";
  if(!$disabled){
    $form['audition_signup_fieldset']['submit'] = array(
      '#type' => 'submit',
      '#weight' => 99,
      '#attributes' => array('class' => array('audition_signup_submit')),
      '#value' => $submit_text,
    );
    if($availability != null){
      $banner = "<span class='auditioneer_audition_signup_banner_text' style='background-color:".variable_get('auditioneer_signup_signedup_color').";'>".variable_get('auditioneer_signup_signedup_banner', "")."</div>";
      $form['audition_signup_fieldset']['info_panel']['#suffix'] .= "<div class='auditioneer_audition_infoblock'><div class='auditioneer_audition_deadline'>$deadline</div><div class='auditioneer_audition_message'>".variable_get('auditioneer_signup_signedup_message', "")."</div></div>";
    } else {
      $banner = "<span class='auditioneer_audition_signup_banner_text' style='background-color:".variable_get('auditioneer_signup_open_color').";'>".variable_get('auditioneer_signup_open_banner', "")."</div>";
      $form['audition_signup_fieldset']['info_panel']['#suffix'] .= "<div class='auditioneer_audition_infoblock'><div class='auditioneer_audition_deadline'>$deadline</div><div class='auditioneer_audition_message'>".variable_get('auditioneer_signup_open_message', "")."</div></div>";
    }
  } else{
    if($availability != null){
      $banner = "<span class='auditioneer_audition_signup_banner_text' style='background-color:".variable_get('auditioneer_signup_signedup_color').";'>".variable_get('auditioneer_signup_signedup_banner', "")."</div>";
      $form['audition_signup_fieldset']['info_panel']['#suffix'] .= "<div class='auditioneer_audition_infoblock'><div class='auditioneer_audition_deadline'>$deadline</div><div class='auditioneer_audition_message'>".variable_get('auditioneer_signup_closed_message', "")."</div></div>";
    } else {
      $banner = "<span class='auditioneer_audition_signup_banner_text' style='background-color:".variable_get('auditioneer_signup_closed_nosignup_color').";'>".variable_get('auditioneer_signup_closed_banner', "")."</div>";
      $form['audition_signup_fieldset']['info_panel']['#suffix'] .= "<div class='auditioneer_audition_infoblock'><div class='auditioneer_audition_deadline'>$deadline</div><div class='auditioneer_audition_message'>".variable_get('auditioneer_signup_closed_nosignup_message', "")."</div></div>";
    }
  }
  $form['audition_signup_fieldset']['info_panel']['#prefix'] = "<div class='auditioneer_audition_signup_banner'>$banner</div>";
  $current_day = "";
  $aud_type_opts = array();
  $have_generic_slots = false;

  // Loop through once to tally up all the types of specialized audition slots
  // in play.
  $count = 0;
  $realitems = array();
  while(array_key_exists($count, $timeblocks)){
    $realitem = array_pop($timeblocks[$count]['entity']['field_collection_item']);
    array_push($realitems, $realitem);
    if(array_key_exists('field_auditioneer_restrict_slot', $realitem)){
      foreach ($realitem['field_auditioneer_restrict_slot']['#items'] as $item){
        $aud_type_opts[$item['value']] = $item['value'];
      }
    } else {
      $have_generic_slots = true;
    }
    $count++;
  }
  if($have_generic_slots){
    if(count($aud_type_opts) == 0){
      $have_generic_slots = false;
    } else {
      $aud_type_opts["General"] = "General";
    }
  }
  usort($realitems, '_auditioneer_timeblock_sort');

  $count = 0;
  foreach ($realitems as $count=> $realitem){
    $checked = 0;
    $paintbox_class = "auditioneer_audition_timeblock_paint_unchecked";
    $date_disabled = false;
    $matches_selected_slot_type = false;
    $input_classes = array('auditioneer_audition_timeblock_checkbox', "${nid}_auditioneer_slot_type_checkbox");
    if(array_key_exists('field_auditioneer_restrict_slot', $realitem)){
      foreach ($realitem['field_auditioneer_restrict_slot']['#items'] as $item){
        $slottype_class = $nid.'_auditioneer_slot_type_'.preg_replace("/[^a-z0-9]/", "_", strtolower($item['value']));
        array_push($input_classes, $slottype_class);
        $paintbox_class .= " $slottype_class";
        if(!empty($slot_type) && $item['value'] == $slot_type){
          $matches_selected_slot_type = true;
        }
      }
    }
    if(count($aud_type_opts) > 0 && !$have_generic_slots && !$matches_selected_slot_type){
      $date_disabled = true;
    }
    if($availability && !$date_disabled){
      foreach ($availability as $ext_req){
        if($ext_req == $realitem['field_auditioneer_timerange']['#items'][0]){
          $checked = 1;
          $paintbox_class = "auditioneer_audition_timeblock_paint_checked";
          break;
        }
      }
    }
    $paintbox_class .= " ${nid}_auditioneer_slot_type_paint";
    $times = field_view_field('field_collection_item', $realitem['field_auditioneer_timerange']['#object'], 'field_auditioneer_timerange', 'auditioneer_audition_signup_form');
    $day = field_view_field('field_collection_item', $realitem['field_auditioneer_timerange']['#object'], 'field_auditioneer_timerange', 'full');
    $current_day = strip_tags($day[0]['#markup']);
    $print_day = "";
    if(!array_key_exists($current_day, $form['audition_signup_fieldset'])){
      $print_day = "<h4 class='auditioneer_audition_timeblock_day_title'>$current_day</h4>";
      $form['audition_signup_fieldset'][$current_day] = array(
        '#type' => 'fieldset',
        '#weight' => 5+$count,
        '#collapsible' => FALSE,
        '#collapsed' => FALSE,
        '#attributes' => array('class' => array('auditioneer_audition_timeblock_day')),
      );
    }
    $time_string = $times[0]['#markup'];

    if(array_key_exists('field_auditioneer_alternate_name', $realitem)){
      $time_string = $realitem['field_auditioneer_alternate_name'][0]['#markup'];
    }
    if($date_disabled){
      $paintbox_class .= " auditioneer_audition_timeblock_paint_disabled";
    }
    $form['audition_signup_fieldset'][$current_day]['timeblock_'.$count] = array(
      '#type' => 'checkbox',
      '#weight' => $count,
      '#disabled' => $date_disabled,
      '#name' => $nid.'_timeblock_'.$count,
      '#attributes' => array('class' => $input_classes),
      '#prefix' => "$print_day<div id='${nid}_timeblock_$count' class='auditioneer_audition_timeblock'><div class='auditioneer_audition_timeblock_paint $paintbox_class'>".$time_string."</div>",
      '#suffix' => "</div>"
    );
    if(!$date_disabled){
      $form['audition_signup_fieldset'][$current_day]['timeblock_'.$count]['#default_value'] = $checked;
    }

  }
  if(count($aud_type_opts) > 0){
    $form['audition_signup_fieldset'][$nid.'_auditioneer_slot_type'] = array(
      '#type' => 'radios',
      '#title' => t("Type of audition (choose one):"),
      '#required' => true,
      '#options' => $aud_type_opts,
      '#prefix' => "<div class='auditioneer_audition_slot_type_radios'>",
      '#suffix' => "</div>",
      '#attributes' => array('class' => array('auditioneer_audition_slot_type_option')),
    );
    if(!empty($slot_type)){
      $form['audition_signup_fieldset'][$nid.'_auditioneer_slot_type']['#default_value'] = $slot_type;
    } elseif($have_generic_slots){
      $form['audition_signup_fieldset'][$nid.'_auditioneer_slot_type']['#default_value'] = "General";
    }
  }

  return($form);
}


function auditioneer_forms($form_id, $args) {
  _auditioneer_display_bits();
  if(!preg_match("/^(audition_signup|audition_mgmt)_\d+$/", $form_id) || count($args) != 2){
    return;
  }
  $nid = $args[0];
  $audition = $args[1];

  $forms[$form_id] = array(
    'callback' => '_auditioneer_form_builder',
    'callback arguments' => array($args)
  );
  return $forms;
}


function auditioneer_audition_signup_validate_callback($form, &$form_state){
  $count = 0;
  $timeblocks = array();
  $nid = $form_state['input']['audition_id'];
  $audition = node_load($form_state['input']['audition_id']);
  $timeblocks = field_view_field('node', $audition, 'field_auditioneer_time_blocks');
  while (array_key_exists($count, $timeblocks)){
    if(array_key_exists("${nid}_timeblock_$count", $form_state['input'])){
      $timeblocks[] = $count;
    }
    $count++;
  }
  if(count($timeblocks) == 0){
    form_set_error("", "You must sign up for at least one block of time");
  }
  return $form;
}


function auditioneer_audition_signup_submit_callback($form, &$form_state){
  global $user;
  print $user->uid;
  $nid = $form_state['input']['audition_id'];
  $audition = node_load($form_state['input']['audition_id']);
  $timeblocks = field_view_field('node', $audition, 'field_auditioneer_time_blocks');
  $requested_blocks = array();
  $count = 0;
  $slot_type = null;
  while (array_key_exists($count, $timeblocks)){
    if(array_key_exists("${nid}_timeblock_$count", $form_state['input'])){
      $item = array_pop($timeblocks[$count]['entity']['field_collection_item']);
      $requested_blocks[$count] = $item['field_auditioneer_timerange']['#items'][0];
    }
    $count++;
  }
  if(array_key_exists("${nid}_auditioneer_slot_type", $form_state['input'])){
    $slot_type = $form_state['input']["${nid}_auditioneer_slot_type"];
  }
  db_delete('auditioneer_signups')
    ->condition('nid', $form_state['input']['audition_id'])
    ->condition('uid', $user->uid)
    ->execute();
  db_insert('auditioneer_signups')
    ->fields(array(
      'nid' => $form_state['input']['audition_id'],
      'uid' => $user->uid,
      'slot_type' => $slot_type,
      'availability' => serialize($requested_blocks)
    ))
    ->execute();
  return $form;
}

function auditioneer_field_widget_form_alter(&$element, &$form_state, $context) {
  _auditioneer_display_bits();
  // Add custom validation hooks for Field Collections. Horrible.
  if ($context['field']['field_name'] == 'field_auditioneer_time_blocks') {
    array_unshift($element['#element_validate'], 'auditioneer_field_auditioneer_time_blocks_validate');
    $new_tr_validate = array(); // XXX strip out date_combo_validate and call explicitly from our own callback, *after* we've fudged the values
    foreach ($element['field_auditioneer_timerange'][LANGUAGE_NONE][0]['#element_validate'] as $callback){
      if($callback != "date_combo_validate"){
        $new_tr_validate[] = $callback;
      }
    }
    $element['field_auditioneer_timerange'][LANGUAGE_NONE][0]['#element_validate'] = $new_tr_validate;
  }
}

/*
function function auditioneer_views_default_views(){
}
 */

/*
function auditioneer_views_pre_render(&$view){
  dpm("pre_render");
  dpm($view);
}

function auditioneer_views_post_render(&$view){
  dpm("post_render");
  dpm($view);
}
 */

function auditioneer_page_alter(&$page){
  _auditioneer_display_bits();
  if($page && array_key_exists('content', $page) && array_key_exists('system_main', $page['content']) && array_key_exists('nodes', $page['content']['system_main'])){
    foreach ($page['content']['system_main']['nodes'] as $nid => $audition){
      if($audition && is_array($audition) && $audition['#bundle'] == "auditioneer_audition"){
        $formbits = auditioneer_signup_page($nid, $audition);
        $page['content']['system_main']['nodes'][$nid] = $formbits;
      }
    }
  }
}
