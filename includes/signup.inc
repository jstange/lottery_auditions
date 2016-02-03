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
  $timeblocks = field_view_field('node', $audition, 'field_auditioneer_time_blocks', 'full');
  $count = $start = $end = 0;
  while(array_key_exists($count, $timeblocks)){
    $realitem = array_pop($timeblocks[$count]['entity']['field_collection_item']);
    $time = strtotime($realitem['field_auditioneer_timerange']['#items'][0]['value']);
    if($start == 0 || $time < $start){
      $start = $time;
    }
    if($end == 0 || $time > $end){
      $end = $time;
    }
    $count++;
  }
  if($start > 0 && $end > 0){
    $_auditioneer_cached_times[$audition->nid] = array($start, $end);
  }
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
    if($output['#markup'] == ""){
      $output['#markup'] = "<h2>There are no upcoming auditions at this time</h2>";
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
  $finalized = false;
  if($audition->field_auditioneer_finalized[LANGUAGE_NONE][0]['value'] > 0){
    $finalized = true;
  }
  $ext_requests = db_select('auditioneer_signups', 's')
    ->fields('s', array('availability', 'contact_data', 'placement', 'slot_type'))
    ->condition('nid', $nid)
    ->condition('uid', $user->uid)
    ->execute()
    ->fetchAssoc();
  $availability = $contact_data = $slot_type = null;
  $submit_text = t('Sign Up');
  if(count($ext_requests) > 0 && is_array($ext_requests)){
    $slot_type = $ext_requests['slot_type'];
    if(array_key_exists('availability', $ext_requests)){
      $availability = unserialize($ext_requests['availability']);
      if(count($availability)  > 0){
        $submit_text = t('Update');
      }
    }
    if(array_key_exists('contact_data', $ext_requests)){
      $contact_data = unserialize($ext_requests['contact_data']);
    }
  }
  $req_contact_fields = Array();
  $avail_contact_fields = Array();
  $contact_items = field_view_field('node', $audition, 'field_auditioneer_collect', 'auditioneer_audition_signup_form');
  if(is_array($contact_items) && array_key_exists('#items', $contact_items) && is_array($contact_items['#items'])){
    foreach ($contact_items['#items'] as $delta => $value){
      $req_contact_fields[] = $value['value'];
      $avail_contact_fields[] = $value['value'];
    }
  }

  $timeblocks = field_view_field('node', $audition, 'field_auditioneer_time_blocks', 'auditioneer_audition_signup_form');
  $address = "<div class='auditioneer_audition_address'>".drupal_render(field_view_field('node', $audition, 'field_auditioneer_location', 'auditioneer_audition_signup_form'))."</div>";
  $instructions = "";
  if(!$finalized){
    $instructions = "<div class='auditioneer_audition_instructions'>".preg_replace("/\n/", "<br />", drupal_render(field_view_field('node', $audition, 'field_auditioneer_instructions', 'auditioneer_audition_signup_form')))."</div>";
  }
  $deadline = drupal_render(field_view_field('node', $audition, 'field_auditioneer_deadline', 'auditioneer_audition_signup_form'));
  $deadline_str = field_view_field('node', $audition, 'field_auditioneer_deadline', 'auditioneer_audition_signup_form');
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
    ),
    'contact_panel' => array(
      '#type' => 'fieldset',
      '#collapsed' => FALSE,
      '#collapsible' => FALSE,
      '#attributes' => array('class' => array('auditioneer_audition_signup_contact_panel')),
    )
  );

  if(count($ext_requests) > 0 && is_array($ext_requests)){
    $form['audition_signup_fieldset']['optout'] = array(
      '#type' => 'submit',
      '#weight' => 98,
      '#attributes' => array('class' => array('audition_signup_submit', 'audition_signup_cancel')),
      '#value' => "Cancel"
    );
  }

  $form['#attributes'] = array('class' => array('auditioneer_audition_signup'));
  $form['#submit'] = array('auditioneer_audition_signup_submit_callback');
  $form['#validate'] = array('auditioneer_audition_signup_validate_callback');
//  list($start, $end) = _auditioneer_get_times($audition);
  $banner = "";
  if($finalized && count($ext_requests) > 0 && is_array($ext_requests)){
    $msg = "";
    $info = "";
    if($ext_requests['placement'] > 0){
      $slot_map = _auditioneer_get_slot_map($audition);
      $msg = "Your audition: ".$slot_map[$ext_requests['placement']];
      $info = drupal_render(field_view_field('node', $audition, 'field_auditioneer_granted_info', 'auditioneer_audition_signup_form'));
    } elseif($ext_requests['placement'] == 0){
      $msg = "You have been placed on the Waitlist for this audition.";
      $info = drupal_render(field_view_field('node', $audition, 'field_auditioneer_waitlist_info', 'auditioneer_audition_signup_form'));
    } elseif($ext_requests['placement'] < 0){
      $msg = "You are currently not eligible for this audition.";
      $info = drupal_render(field_view_field('node', $audition, 'field_auditioneer_ineligible_info', 'auditioneer_audition_signup_form'));
    }
    $info = preg_replace("/\n/", "<br />", $info);
    $form['audition_signup_fieldset']['info_panel']['#suffix'] .= "<div class='auditioneer_audition_infoblock'><div class='auditioneer_audition_deadline'></div><div class='auditioneer_audition_detailed_message'><p><strong>$msg</strong></p>$info</div></div>";
  } elseif(!$disabled){
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
      $banner = "<span class='auditioneer_audition_signup_banner_text' style='background-color:".variable_get('auditioneer_signup_closed_nosignup_color').";'>".variable_get('auditioneer_signup_closed_nosignup_banner', "")."</div>";
      $form['audition_signup_fieldset']['info_panel']['#suffix'] .= "<div class='auditioneer_audition_infoblock'><div class='auditioneer_audition_deadline'>$deadline</div><div class='auditioneer_audition_message'>".variable_get('auditioneer_signup_closed_nosignup_message', "")."</div></div>";
    }
  }
  $form['audition_signup_fieldset']['info_panel']['#prefix'] = "<div class='auditioneer_audition_signup_banner'>$banner</div>";
  $current_day = "";
  $aud_type_opts = array();
  $have_generic_slots = false;
  if($finalized){
    return($form);
  }

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
    $contact_fields = array();
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
    if($disabled){
      $date_disabled = true;
    }
    if(array_key_exists("field_auditioneer_collect", $realitem)){
      foreach ($realitem['field_auditioneer_collect']['#items'] as $delta => $value){
        if($checked == 1){
          $req_contact_fields[] = $value['value'];
        }
        if($value['value'] == "Name"){
          $contact_fields[] = ".form-item-name";
        } elseif($value['value'] == "Email"){
          $contact_fields[] = ".form-item-email";
        } elseif($value['value'] == "Phone"){
          $contact_fields[] = ".form-item-phone";
        } elseif($value['value'] == "Photo/Resume"){
          $contact_fields[] = ".form-item-files-resume";
        }
        $avail_contact_fields[] = $value['value'];
      }
    }

    $paintbox_class .= " ${nid}_auditioneer_slot_type_paint";
    $times = field_view_field('field_collection_item', $realitem['field_auditioneer_timerange']['#object'], 'field_auditioneer_timerange', 'auditioneer_audition_signup_form');
    $day = field_view_field('field_collection_item', $realitem['field_auditioneer_timerange']['#object'], 'field_auditioneer_timerange');
    $current_day = preg_replace("/ - .*/", "", strip_tags($day[0]['#markup']));
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
      '#attributes' => array('class' => $input_classes, 'req_contact_fields' => implode(" ", $contact_fields)),
      '#prefix' => "$print_day<div id='${nid}_timeblock_$count' class='auditioneer_audition_timeblock'><div class='auditioneer_audition_timeblock_paint $paintbox_class' req_contact_fields='".implode(" ", $contact_fields)."'>".$time_string."</div>",
      '#suffix' => "</div>"
    );

    if(!$date_disabled){
      $form['audition_signup_fieldset'][$current_day]['timeblock_'.$count]['#default_value'] = $checked;
      if($checked == 1){
        $form['audition_signup_fieldset'][$current_day]['timeblock_'.$count]['#attributes']["checked"] = "checked";
      }
    }

  }
  if(count($aud_type_opts) > 0){
    $form['audition_signup_fieldset'][$nid.'_auditioneer_slot_type'] = array(
      '#type' => 'radios',
      '#title' => t("Type of audition (choose one):"),
      '#required' => true,
      '#disabled' => $disabled,
      '#options' => $aud_type_opts,
      '#prefix' => '<div class="auditioneer_audition_slot_type_radios">',
      '#suffix' => "</div>",
      '#attributes' => array('class' => array('auditioneer_audition_slot_type_option')),
    );
    if(!empty($slot_type)){
      $form['audition_signup_fieldset'][$nid.'_auditioneer_slot_type']['#default_value'] = $slot_type;
    } elseif($have_generic_slots){
      $form['audition_signup_fieldset'][$nid.'_auditioneer_slot_type']['#default_value'] = "General";
    }
  }

  if(in_array("Name", $avail_contact_fields)){
    $req_class = "";
    if(!in_array("Name", $req_contact_fields)){
      $req_class = "hidden";
    }
    $form['audition_signup_fieldset']['contact_panel']['name'] = array(
      '#type' => 'textfield',
      '#size' => 25,
      '#name' => 'name',
      '#disabled' => $disabled,
      '#title' => t('Name').' <span class="form-required '.$req_class.'" title="This field is required.">*</span>',
      '#weight' => 90,
      '#attributes' => array('class' => array('auditioneer_signup_contact_name'))
    );
    if(!empty($contact_data['name'])){
      $form['audition_signup_fieldset']['contact_panel']['name']['#default_value'] = $contact_data['name'];
      $form['audition_signup_fieldset']['contact_panel']['name']['#value'] = $contact_data['name'];
    }
  }
  if(in_array("Email", $avail_contact_fields)){
    $req_class = "";
    if(!in_array("Email", $req_contact_fields)){
      $req_class = "hidden";
    }
    $form['audition_signup_fieldset']['contact_panel']['email'] = array(
      '#type' => 'textfield',
      '#size' => 25,
      '#disabled' => $disabled,
      '#name' => 'email',
      '#title' => t('Email').' <span class="form-required '.$req_class.'" title="This field is required.">*</span>',
      '#weight' => 91,
      '#attributes' => array('class' => array('auditioneer_signup_contact_email'))
    );
    if(!empty($contact_data['email'])){
      $form['audition_signup_fieldset']['contact_panel']['email']['#default_value'] = $contact_data['email'];
      $form['audition_signup_fieldset']['contact_panel']['email']['#value'] = $contact_data['email'];
    }
  }
  if(in_array("Phone", $avail_contact_fields)){
    $req_class = "";
    if(!in_array("Phone", $req_contact_fields)){
      $req_class = "hidden";
    }
    $form['audition_signup_fieldset']['contact_panel']['phone'] = array(
      '#type' => 'textfield',
      '#name' => 'phone',
      '#size' => 16,
      '#disabled' => $disabled,
      '#title' => t('Phone').' <span class="form-required '.$req_class.'" title="This field is required.">*</span>',
      '#weight' => 92,
      '#attributes' => array('class' => array('auditioneer_signup_contact_phone'))
    );
    if(!empty($contact_data['phone'])){
      $form['audition_signup_fieldset']['contact_panel']['phone']['#default_value'] = $contact_data['phone'];
      $form['audition_signup_fieldset']['contact_panel']['phone']['#value'] = $contact_data['phone'];
    }
  }
  if(in_array("Photo/Resume", $avail_contact_fields)){
    $req_class = "";
    if(!in_array("Photo/Resume", $req_contact_fields)){
      $req_class = "hidden";
    }
    $form['audition_signup_fieldset']['contact_panel']['contact_resume'] = array(
      '#type' => 'file',
      '#name' => 'files[resume]',
      '#disabled' => $disabled,
      '#title' => t('Upload Photo/Resum&eacute;').' <span class="form-required '.$req_class.'" title="This field is required.">*</span>',
      '#weight' => 93,
      '#upload_validators' => array("file_validate_extensions" => array("pdf doc docx jpg jpeg odt rtf")),
      '#attributes' => array('class' => array('auditioneer_signup_contact_resume'))
    );
    if(!empty($contact_data['resume'])){
      $upload_append = "";
      if(!$disabled){
        $upload_append = "<br />You may upload a replacement if desired.";
      }
      $form['audition_signup_fieldset']['contact_panel']['contact_resume']['#title'] = t('Photo/Resum&eacute; already uploaded as <strong>'.preg_replace("/.*?\//", "", $contact_data['resume'])."</strong>$upload_append");
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
  global $user;
  $count = 0;
  $selected_blocks = array();
  $nid = $form_state['input']['audition_id'];
  $audition = node_load($form_state['input']['audition_id']);
  if($form_state['input']['op'] == "Cancel"){
    return($form);
  }

  $req_contact_fields = Array();
  $contact_items = field_view_field('node', $audition, 'field_auditioneer_collect', 'auditioneer_audition_signup_form');
  if(is_array($contact_items) && array_key_exists('#items', $contact_items)){
    foreach ($contact_items['#items'] as $delta => $value){
      $req_contact_fields[] = $value['value'];
    }
  }


  $timeblocks = field_view_field('node', $audition, 'field_auditioneer_time_blocks', 'auditioneer_audition_signup_form');
  while (array_key_exists($count, $timeblocks)){
    if(array_key_exists("${nid}_timeblock_$count", $form_state['input'])){
      $selected_blocks[] = $count;
      $realitem = array_pop($timeblocks[$count]['entity']['field_collection_item']);
      if(isset($realitem['field_auditioneer_collect']['#items'])){
        foreach ($realitem['field_auditioneer_collect']['#items'] as $delta => $value){
          $req_contact_fields[] = $value['value'];
        }
      }
    }
    $count++;
  }
  if(count($selected_blocks) == 0){
    form_set_error("", "You must sign up for at least one block of time");
  }

  if(in_array("Email", $req_contact_fields) && empty($form_state['input']['email'])){
    form_set_error("email", "Email field is required for one or more selected time blocks.");
  }
  if(!empty($form_state['input']['email']) && !valid_email_address($form_state['input']['email'])){
    form_set_error("email", "Email field value '".$form_state['input']['email']."' does not appear to be a valid email address.");
  }

  if(in_array("Name", $req_contact_fields) && empty($form_state['input']['name'])){
    form_set_error("name", "Name field is required for one or more selected time blocks.");
  }
  if(in_array("Phone", $req_contact_fields) && empty($form_state['input']['phone'])){
    form_set_error("phone", "Phone field is required for one or more selected time blocks.");
  }

  if(in_array("Photo/Resume", $req_contact_fields)){
    $dir = variable_get('auditioneer_resume_upload_path').'/auditioneer_'.$nid;
    if(!file_prepare_directory($dir)){
      drupal_mkdir($dir);
    }
    $file = file_save_upload('resume', array("file_validate_extensions" => array("pdf doc docx jpg jpeg odt rtf")), $dir);
    if(empty($file)){
      $contact_data = array();
      $ext_requests = db_select('auditioneer_signups', 's')
        ->fields('s', array('contact_data'))
        ->condition('nid', $nid)
        ->condition('uid', $user->uid)
        ->execute()
        ->fetchAssoc();
      if(count($ext_requests) > 0 && is_array($ext_requests)){
        if(array_key_exists('contact_data', $ext_requests)){
          $contact_data = unserialize($ext_requests['contact_data']);
        }
      }
      if(empty($contact_data['resume'])){
        form_set_error("resume", "Photo/Resum&eacute; field is required for one or more selected time blocks.");
      }
    }
  }
  return $form;
}


function auditioneer_audition_signup_submit_callback($form, &$form_state){
  global $user;
  print $user->uid;
  $nid = $form_state['input']['audition_id'];
  $audition = node_load($form_state['input']['audition_id']);
  $timeblocks = field_view_field('node', $audition, 'field_auditioneer_time_blocks', 'auditioneer_audition_signup_form');
  $requested_blocks = array();
  $count = 0;
  $slot_type = null;
  
  if($form_state['input']['op'] == "Cancel"){
    db_delete('auditioneer_signups')
      ->condition('nid', $nid)
      ->condition('uid', $user->uid)
      ->execute();
    return($form);
  }

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
  $contact = array();
  if(!empty($form_state['input']['email'])){
    $contact['email'] = $form_state['input']['email'];
  }
  if(!empty($form_state['input']['name'])){
    $contact['name'] = $form_state['input']['name'];
  }
  if(!empty($form_state['input']['phone'])){
    $contact['phone'] = $form_state['input']['phone'];
  }

  $validators = array();
  // XXX This seems not only redundant to _validate, but to get ignored if
  // they're different. Go figure.
  $dir = variable_get('auditioneer_resume_upload_path').'/auditioneer_'.$nid;
  if(!file_prepare_directory($dir)){
    drupal_mkdir($dir);
  }
  $file = file_save_upload('resume', array("file_validate_extensions" => array("pdf doc docx jpg jpeg odt rtf")), $dir);
  if(!empty($file)){
    $contact['resume'] = $file->uri;
  }

  $ext_request = db_select('auditioneer_signups', 's')
    ->fields('s', array('contact_data'))
    ->condition('nid', $nid)
    ->condition('uid', $user->uid)
    ->execute()
    ->fetchAssoc();
  $ext_contact = unserialize($ext_request['contact_data']);
  if(!is_array($ext_contact)){
    $ext_contact = array();
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
      'availability' => serialize($requested_blocks),
      'contact_data' => serialize(array_merge($ext_contact, $contact))
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
