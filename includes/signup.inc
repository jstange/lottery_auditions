<?php

/**
 * Fetch the start and end time for a lottery. Useful for sorting displays,
 * hiding old stuff, etc.
 */

$_lottery_audition_cached_times = array();
function _lottery_audition_get_times($lottery){
  global $_lottery_audition_cached_times;
  if(is_array($_lottery_audition_cached_times) &&
    array_key_exists($lottery->nid, $_lottery_audition_cached_times)){
    return $_lottery_audition_cached_times[$lottery->nid];
  }
  $timeblocks = field_view_field('node', $lottery, 'field_lottery_time_blocks');
  $count = $start = $end = 0;
  while(array_key_exists($count, $timeblocks)){
    $realitem = array_pop($timeblocks[$count]['entity']['field_collection_item']);
    $day = field_view_field('field_collection_item', $realitem['field_time_range']['#object'], 'field_time_range', 'full');
    $time = strtotime(preg_replace('/ to .*/', "", strip_tags($day[0]['#markup'])));
    if($start == 0 || $time < $start){
      $start = $time;
    }
    if($end == 0 || $time > $end){
      $end = $time;
    }
    $count++;
  }
  $_lottery_audition_cached_times[$lottery->nid] = array($start, $end);
  return array($start, $end);
}

function _lottery_audition_sort_by_date($a, $b){
  list($a_start, $a_end) = _lottery_audition_get_times($a);
  list($b_start, $b_end) = _lottery_audition_get_times($b);

  return ($a_start < $b_start) ? 1 : -1;
}

function _lottery_audition_timeblock_sort($a, $b){
  $a_start = strtotime($a['field_time_range']['#items'][0]['value']);
  $b_start = strtotime($b['field_time_range']['#items'][0]['value']);

  return ($a_start > $b_start) ? 1 : -1;
}

function lottery_audition_signup_page($nid = null){
  _audition_display_bits();
  if($nid){
    $lottery = node_load($nid);
    $formbits = drupal_get_form('lottery_signup_'.$nid, $nid, $lottery);
    return($formbits);
  } else {
    $lotteries = node_load_multiple(array(), array('type' => "lottery_audition"));
    uasort($lotteries, '_lottery_audition_sort_by_date');

    $output = array();
    $output['#markup'] = "";
    foreach ($lotteries as $nid => $lottery){
      // Skip auditions that are fully in the past or aren't yet published.
      list($start, $end) = _lottery_audition_get_times($lottery);
      if($end < time() || $lottery->status != NODE_PUBLISHED){
        continue;
      }
      $output['#markup'] .= drupal_render(drupal_get_form('lottery_signup_'.$nid, $nid, $lottery));
    }
    return $output;
  }

}


function actorscenter_lottery_entity_info_alter(&$entity_info) {
  _audition_display_bits();
  $entity_info['node']['view modes']['lottery_audition_signup_form'] = array(
    'label' => t('Lottery Audition Signup Form'),
    'custom settings' => TRUE,
  );
  $entity_info['field_collection_item']['view modes']['lottery_audition_signup_form'] = array(
    'label' => t('Lottery Audition Signup Form'),
    'custom settings' => TRUE,
  );
}


function _actorscenter_lottery_signup_form($form, &$form_state, $args){
  global $user;
  $nid = $args[0];
  $lottery = $args[1];
  $disabled = false;
  $ext_requests = db_select('lottery_audition_signups', 's')
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

  $timeblocks = field_view_field('node', $lottery, 'field_lottery_time_blocks');
  $address = "<div class='lottery_audition_address'>".drupal_render(field_view_field('node', $lottery, 'field_lottery_audition_address'))."</div>";
  $instructions = "<div class='lottery_audition_instructions'>".drupal_render(field_view_field('node', $lottery, 'field_lottery_special_instructio'))."</div>";
  $deadline = drupal_render(field_view_field('node', $lottery, 'field_lottery_audition_signup_de'));
  $deadline_str = field_view_field('node', $lottery, 'field_lottery_audition_signup_de');
  $deadline_time = strtotime($deadline_str['#items'][0]['value']);
  $banner_text = "";
  if(time() >= $deadline_time){
    $disabled = true;
  }
  $form['lottery_signup_fieldset'] = array(
    '#prefix' => "<h2>".t($lottery->title)."</h2>",
    '#type' => 'fieldset',
    '#title' => t($lottery->title),
    '#title_display' => 'before',
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#attributes' => array('class' => array('lottery_audition_signup_wrapper')),
    'lottery_id' => array(
      '#type' => 'hidden',
      '#value' => $nid
    ),
    'info_panel' => array(
      '#type' => 'fieldset',
      '#collapsed' => FALSE,
      '#collapsible' => FALSE,
      '#attributes' => array('class' => array('lottery_audition_signup_dummy_panel')),
      '#suffix' => "<div class='lottery_audition_infoblock'>".$address.$instructions."</div>"
    )
  );
  $form['#attributes'] = array('class' => array('lottery_audition_signup'));
  $form['#submit'] = array('lottery_audition_signup_submit_callback');
  $form['#validate'] = array('lottery_audition_signup_validate_callback');
  //  list($start, $end) = _lottery_audition_get_times($lottery);
  $banner = "";
  if(!$disabled){
    $form['lottery_signup_fieldset']['submit'] = array(
      '#type' => 'submit',
      '#weight' => 99,
      '#attributes' => array('class' => array('lottery_signup_submit')),
      '#value' => $submit_text,
    );
    if($availability != null){
      $banner = "<span class='lottery_audition_signup_banner_text' style='background-color:".variable_get('lottery_auditions_signup_signedup_color').";'>Lottery Pending</div>";
      $form['lottery_signup_fieldset']['info_panel']['#suffix'] .= "<div class='lottery_audition_infoblock'><div class='lottery_audition_deadline'>$deadline</div><div class='lottery_audition_message'>".variable_get('lottery_auditions_signup_signedup_message', "")."</div></div>";
    } else {
      $banner = "<span class='lottery_audition_signup_banner_text' style='background-color:".variable_get('lottery_auditions_signup_open_color').";'>Open</div>";
      $form['lottery_signup_fieldset']['info_panel']['#suffix'] .= "<div class='lottery_audition_infoblock'><div class='lottery_audition_deadline'>$deadline</div><div class='lottery_audition_message'>".variable_get('lottery_auditions_signup_open_message', "")."</div></div>";
    }
  } else{
    if($availability != null){
      $banner = "<span class='lottery_audition_signup_banner_text' style='background-color:".variable_get('lottery_auditions_signup_signedup_color').";'>Lottery Pending</div>";
      $form['lottery_signup_fieldset']['info_panel']['#suffix'] .= "<div class='lottery_audition_infoblock'><div class='lottery_audition_deadline'>$deadline</div><div class='lottery_audition_message'>".variable_get('lottery_auditions_signup_closed_message', "")."</div></div>";
    } else {
      $banner = "<span class='lottery_audition_signup_banner_text' style='background-color:".variable_get('lottery_auditions_signup_closed_nosignup_color').";'>Closed</div>";
      $form['lottery_signup_fieldset']['info_panel']['#suffix'] .= "<div class='lottery_audition_infoblock'><div class='lottery_audition_deadline'>$deadline</div><div class='lottery_audition_message'>".variable_get('lottery_auditions_signup_closed_nosignup_message', "")."</div></div>";
    }
  }
  $form['lottery_signup_fieldset']['info_panel']['#prefix'] = "<div class='lottery_audition_signup_banner'>$banner</div>";
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
    if(array_key_exists('field_restrict_audition_type', $realitem)){
      foreach ($realitem['field_restrict_audition_type']['#items'] as $item){
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
  usort($realitems, '_lottery_audition_timeblock_sort');

  $count = 0;
  foreach ($realitems as $count=> $realitem){
    $checked = 0;
    $paintbox_class = "lottery_audition_timeblock_paint_unchecked";
    $date_disabled = false;
    $matches_selected_slot_type = false;
    $input_classes = array('lottery_audition_timeblock_checkbox', "${nid}_lottery_audition_slot_type_checkbox");
    if(array_key_exists('field_restrict_audition_type', $realitem)){
      foreach ($realitem['field_restrict_audition_type']['#items'] as $item){
        $slottype_class = $nid.'_lottery_audition_slot_type_'.preg_replace("/[^a-z0-9]/", "_", strtolower($item['value']));
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
        if($ext_req == $realitem['field_time_range']['#items'][0]){
          $checked = 1;
          $paintbox_class = "lottery_audition_timeblock_paint_checked";
          break;
        }
      }
    }
    $paintbox_class .= " ${nid}_lottery_audition_slot_type_paint";
    $times = field_view_field('field_collection_item', $realitem['field_time_range']['#object'], 'field_time_range', 'lottery_audition_signup_form');
    $day = field_view_field('field_collection_item', $realitem['field_time_range']['#object'], 'field_time_range', 'full');
    $current_day = strip_tags($day[0]['#markup']);
    $print_day = "";
    if(!array_key_exists($current_day, $form['lottery_signup_fieldset'])){
      $print_day = "<h4 class='lottery_audition_timeblock_day_title'>$current_day</h4>";
      $form['lottery_signup_fieldset'][$current_day] = array(
        '#type' => 'fieldset',
        '#weight' => 5+$count,
        '#collapsible' => FALSE,
        '#collapsed' => FALSE,
        '#attributes' => array('class' => array('lottery_audition_timeblock_day')),
      );
    }
    $time_string = $times[0]['#markup'];

    if(array_key_exists('field_audition_alternate_name', $realitem)){
      $time_string = $realitem['field_audition_alternate_name'][0]['#markup'];
    }
    if($date_disabled){
      $paintbox_class .= " lottery_audition_timeblock_paint_disabled";
    }
    $form['lottery_signup_fieldset'][$current_day]['timeblock_'.$count] = array(
      '#type' => 'checkbox',
      '#weight' => $count,
      '#disabled' => $date_disabled,
      '#name' => $nid.'_timeblock_'.$count,
      '#attributes' => array('class' => $input_classes),
      '#prefix' => "$print_day<div id='${nid}_timeblock_$count' class='lottery_audition_timeblock'><div class='lottery_audition_timeblock_paint $paintbox_class'>".$time_string."</div>",
      '#suffix' => "</div>"
    );
    if(!$date_disabled){
      $form['lottery_signup_fieldset'][$current_day]['timeblock_'.$count]['#default_value'] = $checked;
    }

  }
  if(count($aud_type_opts) > 0){
    $form['lottery_signup_fieldset'][$nid.'_lottery_audition_slot_type'] = array(
      '#type' => 'radios',
      '#title' => t("Type of audition (choose one):"),
      '#required' => true,
      '#options' => $aud_type_opts,
      '#prefix' => "<div class='lottery_audition_slot_type_radios'>",
      '#suffix' => "</div>",
      '#attributes' => array('class' => array('lottery_audition_slot_type_option')),
    );
    if(!empty($slot_type)){
      $form['lottery_signup_fieldset'][$nid.'_lottery_audition_slot_type']['#default_value'] = $slot_type;
    } elseif($have_generic_slots){
      $form['lottery_signup_fieldset'][$nid.'_lottery_audition_slot_type']['#default_value'] = "General";
    }
  }

  return($form);
}


function actorscenter_lottery_forms($form_id, $args) {
  _audition_display_bits();
  if(!preg_match("/^(lottery_signup|audition_mgmt)_\d+$/", $form_id) || count($args) != 2){
    return;
  }
  $nid = $args[0];
  $lottery = $args[1];

  $forms[$form_id] = array(
    'callback' => '_actorscenter_lottery_form_builder',
    'callback arguments' => array($args)
  );
  return $forms;
}


function lottery_audition_signup_validate_callback($form, &$form_state){
  $count = 0;
  $timeblocks = array();
  $nid = $form_state['input']['lottery_id'];
  $lottery = node_load($form_state['input']['lottery_id']);
  $timeblocks = field_view_field('node', $lottery, 'field_lottery_time_blocks');
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


function lottery_audition_signup_submit_callback($form, &$form_state){
  global $user;
  print $user->uid;
  $nid = $form_state['input']['lottery_id'];
  $lottery = node_load($form_state['input']['lottery_id']);
  $timeblocks = field_view_field('node', $lottery, 'field_lottery_time_blocks');
  $requested_blocks = array();
  $count = 0;
  $slot_type = null;
  while (array_key_exists($count, $timeblocks)){
    if(array_key_exists("${nid}_timeblock_$count", $form_state['input'])){
      $item = array_pop($timeblocks[$count]['entity']['field_collection_item']);
      $requested_blocks[$count] = $item['field_time_range']['#items'][0];
    }
    $count++;
  }
  if(array_key_exists("${nid}_lottery_audition_slot_type", $form_state['input'])){
    $slot_type = $form_state['input']["${nid}_lottery_audition_slot_type"];
  }
  db_delete('lottery_audition_signups')
    ->condition('nid', $form_state['input']['lottery_id'])
    ->condition('uid', $user->uid)
    ->execute();
  db_insert('lottery_audition_signups')
    ->fields(array(
      'nid' => $form_state['input']['lottery_id'],
      'uid' => $user->uid,
      'slot_type' => $slot_type,
      'availability' => serialize($requested_blocks)
    ))
    ->execute();
  return $form;
}

function actorscenter_lottery_field_widget_form_alter(&$element, &$form_state, $context) {
  _audition_display_bits();
  // Add custom validation hooks for Field Collections. Horrible.
  if ($context['field']['field_name'] == 'field_lottery_time_blocks') {
    array_unshift($element['#element_validate'], 'actorscenter_lottery_field_lottery_time_blocks_validate');
    $new_tr_validate = array(); // XXX strip out date_combo_validate and call explicitly from our own callback, *after* we've fudged the values
    foreach ($element['field_time_range'][LANGUAGE_NONE][0]['#element_validate'] as $callback){
      if($callback != "date_combo_validate"){
        $new_tr_validate[] = $callback;
      }
    }
    $element['field_time_range'][LANGUAGE_NONE][0]['#element_validate'] = $new_tr_validate;
  }
}

/*
function function actorscenter_lottery_views_default_views(){
}
 */

/*
function actorscenter_lottery_views_pre_render(&$view){
  dpm("pre_render");
  dpm($view);
}

function actorscenter_lottery_views_post_render(&$view){
  dpm("post_render");
  dpm($view);
}
 */

function actorscenter_lottery_page_alter(&$page){
  _audition_display_bits();
  if($page && array_key_exists('content', $page) && array_key_exists('system_main', $page['content']) && array_key_exists('nodes', $page['content']['system_main'])){
    foreach ($page['content']['system_main']['nodes'] as $nid => $lottery){
      if($lottery && is_array($lottery) && $lottery['#bundle'] == "lottery_audition"){
        $formbits = lottery_audition_signup_page($nid, $lottery);
        $page['content']['system_main']['nodes'][$nid] = $formbits;
      }
    }
  }
}
