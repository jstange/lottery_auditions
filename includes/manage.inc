<?php

/* Given a slot, shove in a random matching user. */
function _auditioneer_assign_slot_to_someone(&$auditioners, $timeblock, $slot_types){
  // favor people who haven't had a slot in the longest time
  shuffle($auditioners);
  usort($auditioners, '_auditioneer_weight_signups');

  $assignee_key = $assignee = null;
  foreach ($auditioners as $key => $req){
    if($slot_types != null && $req['slot_type'] != null && !in_array($req['slot_type'], $slot_types)){
      continue;
    }
    if(!in_array($timeblock, $req['availability'])){
      continue;
    }
    $assignee = $req;
    $assignee_key = $key;
    break;
  }
  unset($auditioners[$assignee_key]);

  return $assignee;
}

/*
 * Given a user, attempt to find them a matching slot. Returns the index of a
 * matching available slot, or 0 if none was found.
 */
function _auditioneer_assign_user_a_slot($audition, $uid){
  $query = db_select('auditioneer_signups', 's');
  $query->join('users', 'u', 's.uid = u.uid');
  $result = $query
    ->fields('s', array('uid', 'availability', 'placement', 'slot_type', 'contact_data'))
    ->fields('u', array('name', 'mail'))
    ->condition('nid', $audition->nid)
    ->execute();
  $placements = array();
  $this_user = null;
  while($req = $result->fetchAssoc()){
    if($req['placement'] != null && $req['placement'] > 0){
      $placements[$req['placement']] = $req;
    }
    if($req['uid'] == $uid){
      $req['availability'] = unserialize($req['availability']);
      $req['contact_data'] = unserialize($req['contact_data']);
      $this_user = $req;
    }
  }
  if(!$this_user){
    return 0;
  }

  $groups = _auditioneer_list_groups($audition);

  $slot_index = 1;
  foreach ($groups as $group){
    list($start, $end, $slots_per_group, $slot_types, $timeblock, $tblock_count) = $group;
    for($i=0; $i < $slots_per_group; $i++){
      if(isset($placements[$slot_index])){
        $slot_index++;
        continue;
      }
      if($slot_types != null && count($slot_types) > 0 && $this_user['slot_type'] != null && !in_array($this_user['slot_type'], $slot_types)){
        $slot_index++;
        continue;
      }
      if(!in_array($tblock_count, $this_user['availability'])){
        $slot_index++;
        continue;
      }
      return $slot_index;
    }
  }
  return 0;
}

function _auditioneer_signup_mgmt_empty_slot($slot_index, $form_state){
  $form['slots']['audition_mgmt_table'][$group_start][$slot_index] = array(
    '#type' => "fieldset",
    '#attributes' => array('class' => array('audition_mgmt_slot_entry')),
  );
}

function _auditioneer_weight_signups($a, $b){
  return count($a["prev_auditions"]) > count($b["prev_auditions"]);
}

function _auditioneer_sort_signups($a, $b){
  $sort_a = preg_replace("/.*? /", "", strtolower($a['name']));
  $sort_b = preg_replace("/.*? /", "", strtolower($b['name']));
  if(array_key_exists("realname", $a)){
    $sort_a = preg_replace("/.*? /", "", strtolower($a['realname']));
  }
  if(array_key_exists("realname", $b)){
    $sort_b = preg_replace("/.*? /", "", strtolower($b['realname']));
  }
  return (strtolower($sort_a) < strtolower($sort_b)) ? -1 : 1;
}

function _auditioneer_add_mgmt_slot_buttons($req, $type, &$slot, $finalized, $slot_num = ""){
  if(!empty($req['contact_data']['resume'])){
    $slot['resume'] = array(
      '#type' => 'item',
      '#markup' => "<a href='".file_create_url($req['contact_data']['resume'])."' target='_blank'><img src='/".drupal_get_path('module', 'auditioneer')."/images/attachment.png' class='audition_signup_mgmt_button'></a>",
      '#weight' => 20,
      '#theme_wrappers' => array(), // extraneous markup begone
      '#attributes' => array(
        'class' => array('audition_signup_mgmt_button')
      )
    );
  }

  $slot_ajax = array(
    'callback' => 'auditioneer_audition_mgmt_submit_callback',
    'method' => 'replace',
    'event' => 'click',
    'effect' => 'fade'
  );

  if($type == "main" || $type == "waitlist"){
    $slot['cancel'] = array(
      '#type' => 'image_button',
      '#name' => 'cancel'.$type.$slot_num,
      '#src' => drupal_get_path('module', 'auditioneer')."/images/x.png",
      '#default_value' => $req['uid'],
      '#ajax' => $slot_ajax,
      '#attributes' => array(
        'class' => array('audition_signup_mgmt_cancel', 'audition_signup_mgmt_button'),
        'slot' => $slot_num
      )
    );
  }

  if(!$finalized && ($type == "main" || $type == "waitlist")){
    $slot['reject'] = array(
      '#type' => 'image_button',
      '#name' => 'reject'.$type.$slot_num,
      '#src' => drupal_get_path('module', 'auditioneer')."/images/disqualify.png",
      '#default_value' => $req['uid'],
      '#ajax' => $slot_ajax,
      '#attributes' => array(
        'class' => array('audition_signup_mgmt_reject', 'audition_signup_mgmt_button'),
        'slot' => $slot_num
      )
    );
  }

  if($type == "main"){
    if($req['placement'] == 0){
      $slot['save'] = array(
        '#type' => 'image_button',
        '#name' => 'save'.$slot_num,
        '#src' => drupal_get_path('module', 'auditioneer')."/images/save.png",
        '#default_value' => $req['uid'],
        '#ajax' => $slot_ajax,
        '#attributes' => array(
          'class' => array('audition_signup_mgmt_save', 'audition_signup_mgmt_button'),
          'slot' => $slot_num
        )
      );
    }
    if(!$finalized){
      $slot['reload'] = array(
        '#type' => 'image_button',
        '#name' => 'reload'.$slot_num,
        '#src' => drupal_get_path('module', 'auditioneer')."/images/reload.png",
        '#default_value' => $slot_num,
        '#ajax' => $slot_ajax,
        '#attributes' => array(
          'class' => array('audition_signup_mgmt_reload', 'audition_signup_mgmt_button'),
          'slot' => $slot_num
        )
      );
    }
  }

  if($type == "waitlist"){
    $slot['promote'] = array(
      '#type' => 'image_button',
      '#name' => 'promote'.$slot_num,
      '#src' => drupal_get_path('module', 'auditioneer')."/images/promote.png",
      '#default_value' => $req['uid'],
      '#ajax' => $slot_ajax,
      '#attributes' => array(
        'class' => array('audition_signup_mgmt_promote', 'audition_signup_mgmt_button'),
        'slot' => $slot_num,
        'alt' => "If there is a matching free slot for this request, save it."
      )
    );
  }

  if($type == "rejected"){
    $slot['unreject'] = array(
      '#type' => 'image_button',
      '#name' => 'unreject'.$slot_num,
      '#src' => drupal_get_path('module', 'auditioneer')."/images/promote.png",
      '#default_value' => $req['uid'],
      '#ajax' => $slot_ajax,
      '#attributes' => array(
        'class' => array('audition_signup_mgmt_unreject', 'audition_signup_mgmt_button'),
        'slot' => $slot_num,
        'alt' => "Put this entry back into the pool of eligible auditioners."
      )
    );
  }
}

function _auditioneer_get_mgmt_slot_fields($req = null, $classes, $id, $show_slottype = false, $slot_index){
  $name = $mail = $slot_type = "";

  $slot = array(
    '#type' => "fieldset",
    '#title_display' => "before",
    '#weight' => $slot_index,
    '#attributes' => array(
      'class' => $classes,
      'id' => array($id)
    )
  );
  if($slot_index){
    $slot['uid'] = array(
      '#type' => 'hidden',
      '#name' => 'uid'.$slot_index
    );
    $slot['username'] = array(
      '#type' => 'hidden',
      '#name' => 'username'.$slot_index
    );
    $slot['name'] = array(
      '#type' => 'hidden',
      '#name' => 'name'.$slot_index
    );
  }
/*
  XXX wait, this doesn't do anything. Make a nice little popup div from these
  values though? Don't forget the phone field.
  $slot['username'] = array(
    '#type' => 'hidden',
    '#name' => 'username'
  );
  $slot['mail'] = array(
    '#type' => 'hidden',
    '#name' => 'mail'
  );
//    $slot['username']['#value'] = $req['name'];
//    $slot['mail']['#value'] = $mail;
 */

  if($req){
    $slot['uid']['#value'] = $req['uid'];
    $slot['username']['#value'] = $req['name'];
    $name = $req['realname'];
    $mail = $req['realmail'];
    if($req['contact_data']){
      if(isset($req['contact_data']['name'])){
        $name = $req['contact_data']['name'];
      }
      if(isset($req['contact_data']['email'])){
        $mail = $req['contact_data']['email'];
      }
    }
    if($req['slot_type'] && $show_slottype){
      $slot_type = " (".$req['slot_type'].")";
    }
    $slot['#title'] = "<div class='audition_mgmt_table_auditioner_label'><a href='/users/".$req['name']."' target='_blank'>".$name."</a>$slot_type</div>";
    $slot['name']['#value'] = $name;
    $slot['popup_panel'] = array(
      '#type' => 'item',
      '#prefix' => "<div class='auditioneer_popup_panel'>",
      '#suffix' => "</div>",
      '#markup' => "<div class='auditioneer_popup_buttons'><img src='/".drupal_get_path('module', 'auditioneer')."/images/pin.png' class='auditioneer_pin' alt='Pinned'> <span>X</span></div>"
    );
    if($req['uid'] > 0){
      $slot['popup_panel']['#markup'] .= "<a href='/users/".$req['name']."' target='_blank'>".$req['realname']."</a>";
    } else {
      $slot['popup_panel']['#markup'] .= "<strong>".$req['realname']."</strong>";
    }
    if(!empty($req['realmail'])){
      $slot['popup_panel']['#markup'] .= " &lt;<a href='mailto:".$req['realmail']."'>".$req['realmail']."</a>&gt;";
    }
    if(!empty($req['realphone'])){
      $slot['popup_panel']['#markup'] .= " - <em><a href='tel:".$req['realphone']."'>".$req['realphone']."</a></em>";
    }
    if(is_array($req['contact_data']) &&
       array_key_exists("resume", $req['contact_data']) &&
       !empty($req['contact_data']['resume'])){
      $slot['popup_panel']['#markup'] .= "<a href='".file_create_url($req['contact_data']['resume'])."' target='_blank'><img src='/".drupal_get_path('module', 'auditioneer')."/images/attachment.png' class='audition_signup_mgmt_button'></a>";
    }
    if(is_array($req['prev_auditions']) && count($req['prev_auditions']) > 0){
      $slot['popup_panel']['#markup'] .= "<p>Previous auditions:<ul>";
      foreach ($req['prev_auditions'] as $prev_aud){
        $audition = node_load($prev_aud['nid']);
        $dates = strip_tags(drupal_render(field_view_field('node', $audition, 'field_auditioneer_time_blocks', 'auditioneer_audition_signup_form')));
        $slot['popup_panel']['#markup'] .= "<li> <strong>$audition->title</strong>";
        if(!empty($prev_aud['slot_type'])){
          $slot['popup_panel']['#markup'] .= " (".$prev_aud['slot_type'].")";
        }
        $slot['popup_panel']['#markup'] .= ", <em>$dates</em>";
      }
      $slot['popup_panel']['#markup'] .= "</ul></p>";
    }
  } else {
    $slot['#title'] = "<div class='audition_mgmt_table_auditioner_label'>&lt;available&gt;</div>";
    unset($slot['username']);
    unset($slot['name']);
    unset($slot['uid']);
    $slot['#attributes']['class'][] = "unassigned";
  }


  return ($slot);
}

function _auditioneer_load_requests($nid){
  $query = db_select('auditioneer_signups', 's');
  $query->join('users', 'u', 's.uid = u.uid');
  $result = $query
    ->fields('s', array('uid', 'availability', 'placement', 'slot_type', 'contact_data'))
    ->fields('u', array('name', 'mail'))
    ->condition('nid', $nid)
    ->execute();
  $requests = array();
  while($req = $result->fetchAssoc()){
    $req['realname'] = $req['name'];
    $req['realmail'] = $req['mail'];
    $req['realphone'] = "";
    $req['availability'] = unserialize($req['availability']);
    $req['contact_data'] = unserialize($req['contact_data']);
    $fn_field = variable_get('auditioneer_firstname_field');
    $ln_field = variable_get('auditioneer_lastname_field');
    if(!empty($fn_field) || !empty($ln_field)){
      $account = user_load($req['uid']);
      $first = $last = "";
      if(property_exists($account, $fn_field)){
        $first = $account->{$fn_field}[LANGUAGE_NONE][0]['value'];
      }
      if(property_exists($account, $ln_field)){
        $last = $account->{$ln_field}[LANGUAGE_NONE][0]['value'];
      }
      $req['realname'] = "$first $last";
    }
    if($req['contact_data']){
      if(isset($req['contact_data']['name'])){
        $req['realname'] = $req['contact_data']['name'];
      }
      if(isset($req['contact_data']['email'])){
        $req['realmail'] = $req['contact_data']['email'];
      }
      if(isset($req['contact_data']['phone'])){
        $req['realphone'] = $req['contact_data']['phone'];
      }
    }
    if(!empty($req['uid'])){
      $query = db_select('auditioneer_signups', 's');
      $prev_auditions = $query
        ->fields('s', array('nid', 'placement', 'slot_type'))
        ->condition('uid', $req['uid'])
        ->condition('nid', $nid, $operator = '!=')
        ->condition('placement', 0, $operator = '>')
        ->execute();
      $req['prev_auditions'] = array();
      while($oldreq = $prev_auditions->fetchAssoc()){
        $req['prev_auditions'][] = $oldreq;
      }
      $requests[] = $req;
    }
  }
  return ($requests);
}

function _auditioneer_mgmt_form($form, &$form_state, $args){
  global $user;
  $nid = $args[0];
  $audition = node_load($nid);
  $finalized = false;
  if($audition->field_auditioneer_finalized[LANGUAGE_NONE][0]['value'] > 0){
    $finalized = true;
  }
/*
  $trace = debug_backtrace();
  foreach ($trace as $stack => $caller){
//  dpm("form callback invoked on $nid by [$stack] ".print_r($caller['function'], true));
  }
 */
  $interval = $audition->field_auditioneer_group_interval[LANGUAGE_NONE][0]['value'];

  $placements = $unplaced = $disqualified = $tentative = array();
  $requests = _auditioneer_load_requests($nid);
  foreach ($requests as $req){
//  while($req = $result->fetchAssoc()){
    if($req['placement'] != null && $req['placement'] > 0){
      $placements[$req['placement']] = $req;
    } elseif($req['placement'] != null && $req['placement'] < 0){
      $disqualified[] = $req;
    } else {
      $unplaced[] = $req;
    }
  }

  $form['#prefix'] = "<div id='audition_signup_mgmt_$nid' class='audition_signup_mgmt_wholeform'><h2>".t("$audition->title signups")."</h2>";
  $form['#suffix'] = "</div>";
  //  $form['#validate'] = array('auditioneer_audition_mgmt_validate_callback');
  $form['audition_id'] = array(
    '#type' => 'hidden',
    '#value' => $nid
  );

  $button_descs = "<div><img src='/".drupal_get_path('module', 'auditioneer')."/images/attachment.png'> = View the uploaded resume of this auditioner, if available.</div>";
  if(!$finalized){
    $button_descs .= "<div><img src='/".drupal_get_path('module', 'auditioneer')."/images/save.png'> = Save the current occupant of this audition slot.</div>
      <div><img src='/".drupal_get_path('module', 'auditioneer')."/images/reload.png'> = Remove this auditioner from their current slot and place them back on the Waitlist.</div>
      <div><img src='/".drupal_get_path('module', 'auditioneer')."/images/disqualify.png'> = Remove this auditioner from their current slot and place them on the Ineligible list.</div>";
  }
  $button_descs .= "<div><img src='/".drupal_get_path('module', 'auditioneer')."/images/promote.png'> = Remove this auditioner from the Ineligible list and place them back on the Waitlist, OR promote them from the Waitlist to the first available matching slot that has not already been saved.</div>
  <div><img src='/".drupal_get_path('module', 'auditioneer')."/images/x.png'> = Cancel this person's audition entirely, typically at their request. A random eligible request from the Waitlist will be promoted in its place, if one exists.</div>";


  $form['instructions'] = array(
    '#type' => 'item',
    '#prefix' => "<div class='audition_signup_mgmt_instructions'>",
    '#suffix' => "</div>"
  );
  $form['instructions']['audition_signup_mgmt_button_descs'] = array(
    '#type' => 'item',
    '#prefix' => "<div class='audition_signup_mgmt_button_descs'>",
    '#suffix' => "</div>",
    '#markup' => $button_descs,
  );
  if(!$finalized){
    $form['instructions']['finalize'] = array(
      '#type' => 'submit',
      '#name' => 'finalize-'.$nid,
      '#weight' => 500,
      //XXX temp for testing
//      '#disabled' => true,
      '#value' => 'FINALIZE',
      '#prefix' => "<div class='audition_signup_mgmt_finalize'>",
      '#suffix' => "<p>Save all remaining <span class='unsaved'>unsaved</span> slots with their occupants, lock this form, and notify all users of their official status, audition times, etc. Do not press until you're ready for auditioners to be contacted!<p></div>",
      '#description' => "Save all occupied slots, lock this form, and notify users by email as to their status.",
      '#submit' => array('auditioneer_audition_mgmt_submit_callback')
    );
  }
  $form['instructions']['bigbuttons'] = array(
    '#type' => 'item',
    '#prefix' => "<div class='audition_signup_mgmt_bigbuttons'>",
    '#suffix' => "</div>"
  );

  if(!$finalized){
    $form['instructions']['bigbuttons']['save'] = array(
      '#type' => 'submit',
      '#name' => 'save-'.$nid,
      '#weight' => 1,
      '#attributes' => array('class' => array('audition_signup_mgmt_bigbutton')),
      '#value' => 'Save All Slot Assignments',
      '#prefix' => "<div class='audition_signup_mgmt_submit'>",
      '#suffix' => "Save all <span class='unsaved'>unsaved</span> slots with current assignments. Slots marked as <span class='unsaved'>unsaved</span> (*) are not yet officially assigned.</div>",
      '#submit' => array('auditioneer_audition_mgmt_submit_callback')
    );
    $form['instructions']['bigbuttons']['unassign'] = array(
      '#type' => 'submit',
      '#name' => 'unassign-'.$nid,
      '#weight' => 2,
      '#attributes' => array('class' => array('audition_signup_mgmt_bigbutton')),
      '#value' => 'Unsave All Saved Slots',
      '#prefix' => "<div class='audition_signup_mgmt_submit'>",
      '#suffix' => " Remove all current assignments, including those that have already been <span class='saved'>saved</span>, and replace them with new (<span class='unsaved'>unsaved</span>) ones randomly selected from the <span class='unassigned'>waitlist</span>.</div>",
      '#submit' => array('auditioneer_audition_mgmt_submit_callback')
    );
    $form['instructions']['bigbuttons']['interval'] = array(
      '#type' => 'textfield',
      '#title' => t("Group Interval (minutes)"),
      '#name' => 'interval',
      '#size' => 4,
      '#maxlength' => 3,
      '#weight' => 3,
      '#default_value' => $interval
    );
    $form['instructions']['bigbuttons']['saveinterval'] = array(
      '#type' => 'submit',
      '#name' => 'saveinterval-'.$nid,
      '#attributes' => array('class' => array('audition_signup_mgmt_bigbutton', 'audition_signup_mgmt_interval')),
      '#value' => 'Update Group Interval',
      '#weight' => 4,
      '#submit' => array('auditioneer_audition_mgmt_submit_callback')
    );
  } else {
    $form['instructions']['bigbuttons']['#markup'] = "<h2>Assignments have been finalized and all users notified.</h2><p><a href='/admin/config/people/auditions/$nid/printable' class='auditioneer_printlink'><img src='/".drupal_get_path('module', 'auditioneer')."/images/printer.png' alt='Print'> Printable View</a>";
  }


  $form['slots'] = array(
    '#type' => 'item',
    '#prefix' => "<div class='audition_signup_mgmt_slots'>",
    '#suffix' => "</div>"
  );
  $form['slots']['audition_mgmt_unassigned_list'] = array(
    '#prefix' => "<div id='audition_mgmt_unassigned_list_$nid' class='audition_mgmt_unassigned_list'>",
    '#suffix' => "</div>",
    '#weight' => 111,
    '#type' => "fieldset"
  );

  $form['slots']['audition_mgmt_unassigned_list']['reject_list'] = array(
    '#prefix' => "<div id='audition_rejectlist_$nid' class='audition_rejectlist'>",
    '#suffix' => "</div>",
    '#tree' => true,
    '#type' => "fieldset",
    '#title' => t("Ineligible (".count($disqualified).")"),
    '#title_display' => "before"
  );
  usort($disqualified, '_auditioneer_sort_signups');
  foreach ($disqualified as $dq_count => $req){
    $classes = array('audition_mgmt_reject_entry', 'reject');
    $id = 'audition_mgmt_reject_entry_'.$nid."_".$dq_count;
    $form['slots']['audition_mgmt_unassigned_list']['reject_list'][$dq_count] = _auditioneer_get_mgmt_slot_fields($req, $classes, $id, true, $dq_count);
    _auditioneer_add_mgmt_slot_buttons($req, "rejected", $form['slots']['audition_mgmt_unassigned_list']['reject_list'][$dq_count], $finalized, $dq_count);
  }


  $form['slots']['audition_mgmt_table'] = array(
    '#prefix' => "<div class='audition_signup_mgmt_form' id='audition_signup_mgmt_form_$nid'>",
    '#suffix' => "</div>",
    '#title' => t("Current Assignments (".count($placements)." saved)"),
    '#title_display' => "before",
    '#type' => "fieldset",
    '#weight' => 110,
    '#attributes' => array(
      'class' => array('audition_mgmt_table')
    )
  );

  shuffle($unplaced); // Randomize the starting point for who gets a slot

  $interval_s = $interval*60;

  $timeblocks = field_view_field('node', $audition, 'field_auditioneer_time_blocks');
  $tblock_count = 0;
  $slot_index = 1;
  $saved_count = count($placements);
  while(array_key_exists($tblock_count, $timeblocks)){
    $realitem = array_pop($timeblocks[$tblock_count]['entity']['field_collection_item']);
    $start = strtotime($realitem['field_auditioneer_timerange']['#items'][0]['value']." ".$realitem['field_auditioneer_timerange']['#items'][0]['timezone_db']);
    $end = strtotime($realitem['field_auditioneer_timerange']['#items'][0]['value2']." ".$realitem['field_auditioneer_timerange']['#items'][0]['timezone_db']);
    $group = 0;
    $block_slots = $realitem['field_auditioneer_block_slots']['#items'][0]['value'];
    $slot_types = array();
    if(array_key_exists('field_auditioneer_restrict_slot', $realitem)){
      foreach ($realitem['field_auditioneer_restrict_slot']['#items'] as $slottype){
        $slot_types[] = $slottype['value'];
      }
    }

    $slots_per_group = ceil($block_slots/(($end - $start)/$interval_s));
    while($start + $group < $end){
      $group_start = $start+$group;
      $group_end = $start+$group + $interval_s;
      if($group_end > $end){
        $group_end = $end;
      }
      $group_start_dt = new AuditioneerDateTime();
      $group_start_dt->setTimestamp($group_start);
      $group_end_dt = new AuditioneerDateTime();
      $group_end_dt->setTimestamp($group_end);

      $form['slots']['audition_mgmt_table'][$group_start] = array(
        '#type' => "fieldset",
        '#tree' => true,
        '#title' => $group_start_dt->format("l F jS g:ia")." - ".$group_end_dt->format("g:ia"),
        '#title_display' => 'before',
        '#attributes' => array('class' => array('audition-mgmt-group-block'))
      );
      if(count($slot_types) > 0){
        $form['slots']['audition_mgmt_table'][$group_start]['#title'] .= " (".implode(", ", $slot_types).")";
      }

      for($i=0; $i<$slots_per_group; $i++){
        $req = null;
        $saved_class = "unsaved";
        if(array_key_exists($slot_index, $placements)){
          $req = $placements[$slot_index];
          $saved_class = "saved";
        } elseif(count($unplaced) > 0){
          $req = _auditioneer_assign_slot_to_someone($unplaced, $tblock_count, $slot_types);
          $tentative[] = $req;
        }
        $classes = array('audition_mgmt_slot_entry', $saved_class);
        $id = 'audition_mgmt_slot_entry_'.$nid."_".$slot_index;
        $form['slots']['audition_mgmt_table'][$group_start][$slot_index] = _auditioneer_get_mgmt_slot_fields($req, $classes, $id, false, $slot_index);

        if($req != null && $nid && $req['uid']){
          _auditioneer_add_mgmt_slot_buttons($req, "main", $form['slots']['audition_mgmt_table'][$group_start][$slot_index], $finalized, $slot_index);
          $req['placement'] = $slot_index;
          $placements[$slot_index] = $req;
        }
        $slot_index++;
      }
      $group += $interval_s;
    }
    $tblock_count++;
  }
  $form['slots']['audition_mgmt_table']['#title'] = t("Current Assignments ($saved_count, ".count($tentative)." unsaved)");

  $wl_count = count($unplaced);

  $form['slots']['audition_mgmt_unassigned_list']['waitlist'] = array(
    '#prefix' => "<div id='audition_waitlist_$nid' class='audition_waitlist'>",
    '#suffix' => "</div>",
    '#tree' => true,
    '#type' => "fieldset",
    '#title' => t("Waitlist ($wl_count)"),
    '#title_display' => "before"
  );
  usort($unplaced, '_auditioneer_sort_signups');
  $wl_displayed = 1;
  foreach ($unplaced as $wl_count => $req){
    if(in_array($req, $tentative)){
      continue;
    }
    if($wl_displayed > 100){
      // a form with too many elements seems to trip up AJAX
      drupal_set_message("Waitlist too large, displaying 100 of ".count($unplaced)." for now", "warning");
      break;
    }
    $classes = array('audition_mgmt_waitlist_entry', 'waitlist');
    $id = 'audition_mgmt_waitlist_entry_'.$nid."_".$wl_count;
    $form['slots']['audition_mgmt_unassigned_list']['waitlist'][$wl_count] = _auditioneer_get_mgmt_slot_fields($req, $classes, $id, true, $wl_count);
    _auditioneer_add_mgmt_slot_buttons($req, "waitlist", $form['slots']['audition_mgmt_unassigned_list']['waitlist'][$wl_count], $finalized, $wl_count);
    $wl_displayed++;
  }

  if($finalized){
    $emails = array();
    foreach ($placements as $req){
      $emails[] = $req['realname']." <".$req['realmail'].">";
    }
    $form['emails_assigned'] = array(
      '#type' => 'textarea',
      '#disabled' => true,
      '#value' => implode(", ", $emails),
      '#weight' => 200,
      '#title' => "Email List: Auditioners with Slots"
    );
    $wl_emails = array();
    foreach ($unplaced as $req){
      $wl_emails[] = $req['realname']." <".$req['realmail'].">";
    }
    $form['emails_waitlisted'] = array(
      '#type' => 'textarea',
      '#disabled' => true,
      '#value' => implode(", ", $wl_emails),
      '#weight' => 201,
      '#title' => "Email List: Waitlisted"
    );
    $dq_emails = array();
    foreach ($disqualified as $req){
      $dq_emails[] = $req['realname']." <".$req['realmail'].">";
    }
    $form['emails_rejected'] = array(
      '#type' => 'textarea',
      '#disabled' => true,
      '#value' => implode(", ", $dq_emails),
      '#weight' => 202,
      '#title' => "Email List: Ineligible"
    );
  }

  return($form);
}

/*
 * Do the math to break Time Blocks into groups based on Group Interval.
 */
function _auditioneer_list_groups($audition){

  $interval_s = $audition->field_auditioneer_group_interval[LANGUAGE_NONE][0]['value']*60;
  $timeblocks = field_view_field('node', $audition, 'field_auditioneer_time_blocks');
  $tblock_count = 0;
  $groups = array();
  while(array_key_exists($tblock_count, $timeblocks)){
    $realitem = array_pop($timeblocks[$tblock_count]['entity']['field_collection_item']);
    $start = strtotime($realitem['field_auditioneer_timerange']['#items'][0]['value']." ".$realitem['field_auditioneer_timerange']['#items'][0]['timezone_db']);
    $end = strtotime($realitem['field_auditioneer_timerange']['#items'][0]['value2']." ".$realitem['field_auditioneer_timerange']['#items'][0]['timezone_db']);
    $group = 0;
    $block_slots = $realitem['field_auditioneer_block_slots']['#items'][0]['value'];
    $slot_types = array();
    if(array_key_exists('field_auditioneer_restrict_slot', $realitem)){
      foreach($realitem['field_auditioneer_restrict_slot']['#items'] as $slot_type){
        $slot_types[] = $slot_type['value'];
      }
    }
    $slots_per_group = ceil($block_slots/(($end - $start)/$interval_s));
    while($start + $group < $end){
      $group_start = $start+$group;
      $group_end = $start+$group + $interval_s;
      if($group_end > $end){
        $group_end = $end;
      }
      if($group_end > $end){
        $group_end = $end;
      }
      $groups[] = array($group_start, $group_end, $slots_per_group, $slot_types, $realitem['field_auditioneer_timerange']['#items'][0], $tblock_count);
      $group += $interval_s;
    }
    $tblock_count++;
  }
  return $groups;
}

function auditioneer_audition_mgmt_validate_callback($form, &$form_state){
  $audition = node_load($form_state['values']['audition_id']);
  $groups = _auditioneer_list_groups($audition);
  return $form;
}

/*
 * Regenerate the whole form. Handy for AJAX commands to invoke.
 */
function _auditioneer_new_form($nid, $form_state){
  $new_state = array();
  $new_state['build_info'] = $form_state['build_info'];
  $new_state['rebuild'] = TRUE;
  $new_state['values'] = array();
  $new_state += form_state_defaults();
  return drupal_rebuild_form('audition_mgmt_'.$nid, $new_state);
}

/*
 * Save every occupied slot, used by the button that does that on purpose as
 * well as by finalization.
 */
function _auditioneer_save_all_slots($groups, $form_state){
  $slot_index = 1;
  $saved = 0;

  foreach ($groups as $group){
    list($start, $end, $slots, $slot_types, $timeblock) = $group;
    for($i=0; $i < $slots; $i++){
      if(!empty($form_state['values'][$start][$slot_index]['uid'])){
        $result = db_update('auditioneer_signups')
          ->fields(array('placement' => $slot_index))
          ->condition('nid', $form_state['values']['audition_id'])
          ->condition('uid', $form_state['values'][$start][$slot_index]['uid'])
          ->execute();
        $saved++;
      } else {
        db_update('auditioneer_signups')
          ->fields(array('placement' => null))
          ->condition('nid', $form_state['values']['audition_id'])
          ->condition('placement', $slot_index)
          ->execute();
      }
      $slot_index++;
    }
  }
  drupal_set_message("Saved $saved slots (".count($groups)." groups).");
}


/**
 * Implements hook_mail()
 */
function auditioneer_mail($key, &$message, $params){
  list($timeslot, $audition, $infoblock) = $params;
  $options['language'] = $message['language'];
  $message['headers']['Content-Type'] = 'text/html; charset=UTF-8; format=flowed';
  $message['headers']['MIME-Version'] = '1.0';
  $message['headers']['Content-Transfer-Encoding'] = '8bit';
  $langcode = $message['language']->language;
  $message['headers']['Bcc'] = $message['to'];
  $message['to'] = "";
  switch($key) {
  case 'placement':
    $message['subject'] = t('Audition slot for '.$audition->title, array(), array('langcode' => $langcode));
//    $message['subject'] = mime_header_encode(t('Audition slot for '.$audition->title, array(), array('langcode' => $langcode)));
    $message['body'][] = t("<p><strong>Your audition time:</strong> $timeslot</p>\n\n", array(), array('langcode' => $langcode));
    $location = preg_replace('/class="field-label"/', 'class="field-label" style="font-weight:bold;"', drupal_render(field_view_field('node', $audition, 'field_auditioneer_location', 'auditioneer_audition_signup_form')));
    $message['body'][] = t("<p>$location</p>\n\n", array(), array('langcode' => $langcode));

    break;
  case 'waitlist':
    $message['subject'] = t('Waitlist for '.$audition->title, array(), array('langcode' => $langcode));
    $message['body'][] = t("<p>You have been placed on the Waitlist for the audition \"$audition->title.\"</p>\n\n", array(), array('langcode' => $langcode));
    break;
  case 'ineligible':
    $message['subject'] = t('Ineligible for '.$audition->title, array(), array('langcode' => $langcode));
    $message['body'][] = t("<p>We're sorry, you are ineligible for the audition \"$audition->title\" at this time.</p>\n\n", array(), array('langcode' => $langcode));
    break;
  case 'admin_notify':
    $message['subject'] = t('Sent Auditioneer slot notifications for '.$audition->title, array(), array('langcode' => $langcode));
    $message['body'][] = t("$timeslot\n", array(), array('langcode' => $langcode));
    break;
  }

  if(!empty($infoblock)){
    $message['body'][] = t($infoblock, array(), array('langcode' => $langcode));
  }
}

/*
 * Generate a simple mapping of slot number to audition time
 */
function _auditioneer_get_slot_map($audition){
  $groups = _auditioneer_list_groups($audition);
  $slot_map = array();
  $slot_index = 1;
  foreach ($groups as $group){
    list($start, $end, $slots_per_group, $slot_types, $timeblock) = $group;
    $group_start_dt = new AuditioneerDateTime();
    $group_start_dt->setTimestamp($start);
    $group_end_dt = new AuditioneerDateTime();
    $group_end_dt->setTimestamp($end);
    for($i=0; $i < $slots_per_group; $i++){
      $slot_map[$slot_index] = $group_start_dt->format("l, F jS, g:ia")." - ".$group_end_dt->format("g:ia");
      $slot_index++;
    }
  }
  return $slot_map;
}

/**
 * Send mail to people who got slots, who are waitlisted, etc
 */
function _auditioneer_notify_users($audition, $users = array(), $reason = ""){
  $slot_map = _auditioneer_get_slot_map($audition);

  $got_slot_recips = array();
  $got_slot_info = preg_replace("/\n/", "<br />\n", drupal_render(field_view_field('node', $audition, 'field_auditioneer_granted_info', 'auditioneer_audition_signup_form')));
  $waitlist_recips = array();
  $waitlist_info = preg_replace("/\n/", "<br />\n", drupal_render(field_view_field('node', $audition, 'field_auditioneer_waitlist_info', 'auditioneer_audition_signup_form')));
  $ineligible_recips = array();
  $ineligible_info = preg_replace("/\n/", "<br />\n", drupal_render(field_view_field('node', $audition, 'field_auditioner_ineligible_info', 'auditioneer_audition_signup_form')));

  $query = db_select('auditioneer_signups', 's');
  $query->join('users', 'u', 's.uid = u.uid');
  $result = $query
    ->fields('s', array('uid', 'availability', 'placement', 'slot_type', 'contact_data'))
    ->fields('u', array('name', 'mail'))
    ->condition('nid', $audition->nid)
    ->execute();

  $from = variable_get('auditioneer_notify_from', variable_get('site_mail', ini_get('sendmail_from')));
//$from = "stange@johnstange.net";
  $messages = array();
  while($req = $result->fetchAssoc()){
    if(count($users) > 0 && !in_array($req['uid'], $users)){
      continue;
    }
    $name = $req['name'];
    $mail = $req['mail'];
    $req['availability'] = unserialize($req['availability']);
    $req['contact_data'] = unserialize($req['contact_data']);
    if($req['contact_data']){
      if(isset($req['contact_data']['name'])){
        $name = $req['contact_data']['name'];
      }
      if(isset($req['contact_data']['email'])){
        $mail = $req['contact_data']['email'];
      }
    }
    // XXX testing
//$mail = "stange@johnstange.net";
    if($req['placement'] != null && $req['placement'] > 0){
      if(!array_key_exists($slot_map[$req['placement']], $got_slot_recips)){
        $got_slot_recips[$slot_map[$req['placement']]] = array();
      }
      $got_slot_recips[$slot_map[$req['placement']]][] = "$name <$mail>";
    } elseif($req['placement'] != null && $req['placement'] < 0){
      if(!empty($ineligible_info)){
        $ineligible_recips[] = "$name <$mail>";
      }
    } else {
      if(!empty($waitlist_info)){
        $waitlist_recips[] = "$name <$mail>";
      }

    }
  }

  if(count($waitlist_recips) > 0){
    $params = array(0, $audition, $waitlist_info);
    $messages[] = drupal_mail("auditioneer", "waitlist", implode(", ", $waitlist_recips), language_default(), $params, $from, true);
  }
  if(count($ineligible_recips) > 0){
    $params = array(0, $audition, $ineligible_info);
    $messages[] = drupal_mail("auditioneer", "ineligible", implode(", ", $ineligible_recips), language_default(), $params, $from, true);
  }
  $got_slot_count = 0;
  if(count($got_slot_recips) > 0){
    foreach ($got_slot_recips as $group => $recips){
      $params = array($group, $audition, $got_slot_info);
      $got_slot_count += count($recips);
      $messages[] = drupal_mail("auditioneer", "placement", implode(", ", $recips), language_default(), $params, $from, true);
    }
  }

  $params = array("$reason<br />\nSent email notifications to $got_slot_count slot recipients, ".count($waitlist_recips)." waitlisters, and ".count($ineligible_recips)." ineligibles.", $audition, "");
  drupal_mail("auditioneer", "admin_notify", $from, language_default(), $params, $from, true);
}

/**
 * Display a simple list of signups for an audition where signups aren't closed
 * yet.
 */
function _auditioneer_simple_list($audition){
  $output = "";

  $entered = _auditioneer_load_requests($audition->nid);

  usort($entered, '_auditioneer_sort_signups');
  $output .= "<h2>Current signups</h2>";
  foreach ($entered as $req){
    if($req['uid'] > 0){
      $output .= "<a href='/users/".$req['name']."' target='_blank'>".$req['realname']."</a>";
    } else {
      $output .= "<strong>".$req['realname']."</strong>";
    }
    if(!empty($req['realmail'])){
      $output .= " &lt;<a href='mailto:".$req['realmail']."'>".$req['realmail']."</a>&gt;";
    }
    if(!empty($req['realphone'])){
      $output .= " - <em><a href='tel:".$req['realphone']."'>".$req['realphone']."</a></em>";
    }
    if(is_array($req['contact_data']) &&
       array_key_exists("resume", $req['contact_data']) &&
       !empty($req['contact_data']['resume'])){
      $output .= "<a href='".file_create_url($req['contact_data']['resume'])."' target='_blank'><img src='/".drupal_get_path('module', 'auditioneer')."/images/attachment.png' class='audition_signup_mgmt_button'></a>";
    }
    $output .= "<br />\n";
  }

  return($output);
}

/**
 * Display a printer-friendly version of our signup list. Intended for
 * auditions that are already finalized.
 */
function _auditioneer_printable($audition){
  $output = "";

  $requests = _auditioneer_load_requests($audition->nid);

  $groups_printed = array();

  $placed = $waitlisted = $ineligible = array();
  foreach ($requests as $req){
    if($req['placement'] != null && $req['placement'] > 0){
      $placed[$req['placement']] = $req;
    } elseif($req['placement'] != null && $req['placement'] < 0){
      $ineligible[] = $req;
    } else {
      $waitlisted[] = $req;
    }
  }

  $groups = _auditioneer_list_groups($audition);
  $slot_map = array();
  $slot_index = 1;
  $output .= "<h2>Auditioners with Assigned Slots</h2>";
  foreach ($groups as $group){
    list($start, $end, $slots_per_group, $slot_types, $timeblock) = $group;
    $group_start_dt = new AuditioneerDateTime();
    $group_start_dt->setTimestamp($start);
    $group_end_dt = new AuditioneerDateTime();
    $group_end_dt->setTimestamp($end);
    $output .= "<h3>".$group_start_dt->format("l, F jS, g:ia")." - ".$group_end_dt->format("g:ia")."</h3>";
    for($i=0; $i < $slots_per_group; $i++){
      $output .= "$slot_index. ";
      if(array_key_exists($slot_index, $placed)){
        $output .= "<strong>".$placed[$slot_index]['realname']."</strong> &lt;".$placed[$slot_index]['realmail']."&gt;";
        if(!empty($placed[$slot_index]['realphone'])){
          $output .= " - <em>".$placed[$slot_index]['realphone']."</em>";
        }
      }
      $output .= "<br />";
      $slot_index++;
    }
  }

  if(count($waitlisted) > 0){
    usort($waitlisted, '_auditioneer_sort_signups');
    $output .= "<h2>Waitlist</h2>";
    foreach ($waitlisted as $req){
      $output .= "<strong>".$req['realname']."</strong> &lt;".$req['realmail']."&gt;";
      if(!empty($req['realphone'])){
        $output .= " - <em>".$req['realphone']."</em>";
      }
      $output .= "<br />";
    }
  }

  if(count($ineligible) > 0){
    usort($ineligible, '_auditioneer_sort_signups');
    $output .= "<h2>Ineligible</h2>";
    foreach ($ineligible as $req){
      $output .= "<strong>".$req['realname']."</strong> &lt;".$req['realmail']."&gt;";
      if(!empty($req['realphone'])){
        $output .= " - <em>".$req['realphone']."</em>";
      }
      $output .= "<br />";
    }
  }

  return($output);
}

/**
 * All actions and buttons on the management form call this guy.
 */
function auditioneer_audition_mgmt_submit_callback($form, &$form_state){
  $nid = $form_state['values']['audition_id'];
  $audition = node_load($form_state['values']['audition_id']);
  $finalized = false;
  if($audition->field_auditioneer_finalized[LANGUAGE_NONE][0]['value'] > 0){
    $finalized = true;
  }
  $groups = _auditioneer_list_groups($audition);
  $form_state['rebuild'] = TRUE;
  // XXX testing
//_auditioneer_notify_users($audition);
  switch ($form_state['triggering_element']['#name']) {
  case "saveinterval-$nid":
    $newinterval = $form_state['input']['interval'];
    if(!is_numeric($newinterval) || $newinterval < 10 || $newinterval > 120){
      form_set_error("interval", "Group Interval <strong>$newinterval</strong> is invalid, must be a number between 10 and 120");
    } else {
      drupal_set_message("Setting interval to <strong>$newinterval</strong> minutes.");
      $audition->field_auditioneer_group_interval[LANGUAGE_NONE][0]['value'] = $newinterval;
      field_attach_update('node', $audition);
      entity_get_controller('node')->resetCache(array($audition->nid));
    }
    return(_auditioneer_new_form($nid, $form_state));
    break;
  case "finalize-$nid":
    _auditioneer_save_all_slots($groups, $form_state);
    $audition->field_auditioneer_finalized[LANGUAGE_NONE][0]['value'] = 1;
    field_attach_update('node', $audition);
    entity_get_controller('node')->resetCache(array($audition->nid));
    _auditioneer_notify_users($audition);
    return(_auditioneer_new_form($nid, $form_state));
    break;
  case "save-$nid":
    _auditioneer_save_all_slots($groups, $form_state);
    return(_auditioneer_new_form($nid, $form_state));
    break;
  case "reassign-$nid":
    // this doesn't actually have to do anything, reloading the form will
    // plant new assignments into unsaved slots
    drupal_set_message("Reassigned all unsaved and empty slots with random selections.");
    return(_auditioneer_new_form($nid, $form_state));
    break;
  case "unassign-$nid":
    drupal_set_message("Removed all assignments from $audition->title", "warning");
    db_update('auditioneer_signups')
      ->fields(array('placement' => null))
      ->condition('nid', $form_state['values']['audition_id'])
      ->condition('placement', 1, '>=')
      ->execute();
    return(_auditioneer_new_form($nid, $form_state));
    break;
  default:
    // These are all AJAX responses.
    $commands = array();
    if(preg_match("/^reject(main|waitlist)\d+/", $form_state['triggering_element']['#name'])){
      db_update('auditioneer_signups')
        ->fields(array('placement' => -1))
        ->condition('nid', $form_state['values']['audition_id'])
        ->condition('uid', $form_state['triggering_element']['#default_value'])
        ->execute();
      $parents = $form_state['triggering_element']['#array_parents'];
      $group = $parents[count($parents)-3];
      $slot_index = $parents[count($parents)-2];
      drupal_set_message("Removed <strong>".$form_state['values'][$group][$slot_index]['name']."</strong> from eligibility for <strong>".$audition->title."</strong>", "warning");
    } elseif(preg_match("/^cancel(main|waitlist)\d+/", $form_state['triggering_element']['#name'])){
      _auditioneer_cancel_signup($nid, $audition, $form_state['triggering_element']['#default_value'], true);
      // _auditioneer_cancel_signup does drupal_set_message on its own
    } elseif(preg_match("/^unreject\d+/", $form_state['triggering_element']['#name'])){
      db_update('auditioneer_signups')
        ->fields(array('placement' => 0))
        ->condition('nid', $form_state['values']['audition_id'])
        ->condition('uid', $form_state['triggering_element']['#default_value'])
        ->execute();
      $parents = $form_state['triggering_element']['#array_parents'];
      $slot_index = $parents[count($parents)-2];
      drupal_set_message("<strong>".$form_state['values']['reject_list'][$slot_index]['name']."</strong> is now eligible for <strong>".$audition->title."</strong>");
    } elseif(preg_match("/^save\d+/", $form_state['triggering_element']['#name'])){
      $parents = $form_state['triggering_element']['#array_parents'];
      $slot_index = $parents[count($parents)-2];
      $group = $parents[count($parents)-3];
      db_update('auditioneer_signups')
        ->fields(array('placement' => $slot_index))
        ->condition('nid', $form_state['values']['audition_id'])
        ->condition('uid', $form_state['triggering_element']['#default_value'])
        ->execute();

      if($finalized){
        _auditioneer_notify_users($audition, array($form_state['triggering_element']['#default_value']), $form_state['values'][$group][$slot_index]['name']." has been granted an audition slot after finalization.");
      }
      drupal_set_message("<strong>".$form_state['values'][$group][$slot_index]['name']."</strong> saved to slot <strong>".$slot_index."</strong>");
    } elseif(preg_match("/^reload\d+/", $form_state['triggering_element']['#name'])){
      $parents = $form_state['triggering_element']['#array_parents'];
      $slot_index = $parents[count($parents)-2];
      db_update('auditioneer_signups')
        ->fields(array('placement' => 0))
        ->condition('nid', $form_state['values']['audition_id'])
        ->condition('placement', $form_state['triggering_element']['#default_value'])
        ->execute();
      drupal_set_message("<strong>".$form_state['values']['waitlist'][$slot_index]['name']."</strong> removed from slot <strong>".$slot_index."</strong> and waitlisted.");
    } elseif(preg_match("/^promote\d+/", $form_state['triggering_element']['#name'])){
      $slot_index = _auditioneer_assign_user_a_slot($audition, $form_state['triggering_element']['#default_value']);
      $parents = $form_state['triggering_element']['#array_parents'];
      $old_slot_index = $parents[count($parents)-2];
      if($slot_index < 1){
        drupal_set_message("Failed to find a matching slot to promote <strong>".$form_state['values']['waitlist'][$old_slot_index]['name']."</strong>", "warning");
      } else {
        db_update('auditioneer_signups')
          ->fields(array('placement' => $slot_index))
          ->condition('nid', $audition->nid)
          ->condition('uid', $form_state['triggering_element']['#default_value'])
          ->execute();
        drupal_set_message("Promoted <strong>".$form_state['values']['waitlist'][$old_slot_index]['name']."</strong> to slot <strong>$slot_index</strong>");
      }
    }
    $commands[] = ajax_command_remove('div.messages');
    $commands[] = ajax_command_before('div.region-content', theme('status_messages'));
    $newform = _auditioneer_new_form($nid, $form_state);
    $commands[] = ajax_command_replace('#audition_signup_mgmt_'.$nid, render($newform));
    return array('#type' => 'ajax', '#commands' => $commands);
    break;
  }
  return(_auditioneer_new_form($nid, $form_state));
}

/**
 * Main management page callback. Invokes other stuff above.
 */
function auditioneer_manage($nid, $mode = null) {
  _auditioneer_display_bits();
  $audition = node_load($nid);
  $output = array();

  if($mode == "printable"){
    return _auditioneer_printable($audition);
  }

  // Skip auditions that have ended, obviously
  list($start, $end) = _auditioneer_get_times($audition);
  if($end < time()){
    $foo = new AuditioneerDateTime();
    $foo->setTimestamp($end);
    $output['#markup'] = $foo->format('l F jS g:ia')."<h1>THIS AUDITION HAS ENDED</h1>".drupal_render(node_view($audition, "full"));
    return $output;
  }

  // If we have an audition that's not past the signup deadline yet, make a
  // note of it but don't provide the full form.
  $deadline = drupal_render(field_view_field('node', $audition, 'field_auditioneer_deadline'));
  $deadline_str = field_view_field('node', $audition, 'field_auditioneer_deadline');
  $deadline_time = strtotime($deadline_str['#items'][0]['value']);
  if(time() < $deadline_time){
    $output['#markup'] = "<h1>SIGNUPS HAVE NOT YET CLOSED</h1>".drupal_render(node_view($audition, "full"));
    $output['#markup'] .= _auditioneer_simple_list($audition);
    return $output;
  }

  return(drupal_get_form('audition_mgmt_'.$nid, $nid, $audition));
}

