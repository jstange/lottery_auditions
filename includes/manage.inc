<?php

function _actorscenter_lottery_assign_slot(&$auditioners, $timeblock, $slot_types){
  // XXX favor people who haven't had a slot in the longest time
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

function _actorscenter_lottery_signup_mgmt_empty_slot($slot_index, $form_state){
  $form['slots']['audition_mgmt_table'][$group_start][$slot_index] = array(
    '#type' => "fieldset",
    '#attributes' => array('class' => array('audition_mgmt_slot_entry')),
  );
}

function _actorscenter_lottery_sort_signups($a, $b){
  return ($a['name'] < $b['name']) ? -1 : 1;
}

function _actorscenter_lottery_mgmt_form($form, &$form_state, $args){
  global $user;
  $nid = $args[0];
  $lottery = $args[1];
/*
  $trace = debug_backtrace();
  foreach ($trace as $stack => $caller){
  dpm("form callback invoked on $nid by [$stack] ".print_r($caller['function'], true));
  }
*/
  $interval = $lottery->field_group_interval[LANGUAGE_NONE][0]['value'];
  $query = db_select('lottery_audition_signups', 's');
  $query->join('users', 'u', 's.uid = u.uid');
  $result = $query
    ->fields('s', array('uid', 'availability', 'placement', 'slot_type'))
    ->fields('u', array('name', 'mail'))
    ->condition('nid', $nid)
    ->execute();
  $placements = $unplaced = $disqualified = array();
  while($req = $result->fetchAssoc()){
    $req['availability'] = unserialize($req['availability']);
    if($req['placement'] != null && $req['placement'] > 0){
      $placements[$req['placement']] = $req;
    } elseif($req['placement'] != null && $req['placement'] < 0){
      $disqualified[] = $req;
    } else {
      $unplaced[] = $req;
    }
  }
  uasort($disqualified, '_actorscenter_lottery_sort_signups');
  uasort($unplaced, '_actorscenter_lottery_sort_signups');

  $form['#prefix'] = "<div id='audition_signup_mgmt_$nid' class='audition_signup_mgmt_wholeform'><h2>".t("$lottery->title signups")."</h2>";
  $form['#suffix'] = "</div>";
  //  $form['#validate'] = array('lottery_audition_mgmt_validate_callback');
  $form['lottery_nid'] = array(
    '#type' => 'hidden',
    '#value' => $nid
  );

  $button_descs = "<div class='audition_signup_mgmt_button_descs'>
    <div><img src='/".drupal_get_path('module', 'actorscenter_lottery')."/images/save.png'> = Save the current occupant of this audition slot.</div>
    <div><img src='/".drupal_get_path('module', 'actorscenter_lottery')."/images/reload.png'> = Remove this auditioner from their current slot and place them back on the Waitlist.</div>
    <div><img src='/".drupal_get_path('module', 'actorscenter_lottery')."/images/x.png'> = Remove this auditioner from their current slot and place them on the Ineligible list.</div>
    <div><img src='/".drupal_get_path('module', 'actorscenter_lottery')."/images/promote.png'> = Remove this auditioner from the Ineligible list and place them back on the Waitlist.</div>
    </div>";

$form['instructions'] = array(
  '#type' => 'item',
  '#prefix' => "<div class='audition_signup_mgmt_instructions'>$button_descs",
  '#suffix' => "</div>"
);
$form['instructions']['bigbuttons'] = array(
  '#type' => 'item',
  '#prefix' => "<div class='audition_signup_mgmt_bigbuttons'>",
  '#suffix' => "</div>"
);

$form['instructions']['bigbuttons']['save'] = array(
  '#type' => 'submit',
  '#name' => 'save-'.$nid,
  '#weight' => 100,
  '#attributes' => array('class' => array('audition_signup_mgmt_bigbutton')),
  '#value' => 'Save All Slot Assignments',
  '#prefix' => "<div class='audition_signup_mgmt_submit'>",
  '#suffix' => "Save all <span class='unsaved'>unsaved</span> slots with current assignments. Slots marked as <span class='unsaved'>unsaved</span> (*) are not yet officially assigned.</div>",
  '#submit' => array('lottery_audition_mgmt_submit_callback')
);
$form['instructions']['bigbuttons']['unassign'] = array(
  '#type' => 'submit',
  '#name' => 'unassign-'.$nid,
  '#weight' => 101,
  '#attributes' => array('class' => array('audition_signup_mgmt_bigbutton')),
  '#value' => 'Unsave All Saved Slots',
  '#prefix' => "<div class='audition_signup_mgmt_submit'>",
  '#suffix' => " Remove all current assignments, including those that have already been <span class='saved'>saved</span>, and replace them with new (<span class='unsaved'>unsaved</span>) ones randomly selected from the <span class='unassigned'>waitlist</span>.</div>",
  '#submit' => array('lottery_audition_mgmt_submit_callback')
);


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

if(count($disqualified) > 0){
  $form['slots']['audition_mgmt_unassigned_list']['reject_list'] = array(
    '#prefix' => "<div id='audition_reject_mgmt_form_$nid' class='audition_reject_mgmt_form'>",
    '#suffix' => "</div>",
    '#tree' => true,
    '#type' => "fieldset",
    '#title' => t("Ineligible (".count($disqualified).")"),
    '#title_display' => "before"
  );
  foreach ($disqualified as $dq_count => $req){
    $slot_type = "";
    if(!empty($req['slot_type'])){
      $slot_type = " (".$req['slot_type'].")";
    }
    $form['slots']['audition_mgmt_unassigned_list']['reject_list'][$dq_count] = array(
      '#type' => "fieldset",
      '#title_display' => "before",
      '#title' => "<div class='audition_mgmt_table_auditioner_label'><a href='/users/".$req['name']."' target='_blank'>".$req['name']."</a>$slot_type</div>",
      '#attributes' => array(
        'class' => array('audition_mgmt_reject_entry', 'reject'),
        'id' => array('audition_mgmt_reject_entry_'.$nid."_".$dq_count)
      ),
    );
    $form['slots']['audition_mgmt_unassigned_list']['reject_list'][$dq_count]['uid'] = array(
      '#type' => 'hidden',
      '#value' => $req['uid'],
      '#name' => 'uid'
    );
    $form['slots']['audition_mgmt_unassigned_list']['reject_list'][$dq_count]['username'] = array(
      '#type' => 'hidden',
      '#value' => $req['name'],
      '#name' => 'username'
    );
    $form['slots']['audition_mgmt_unassigned_list']['reject_list'][$dq_count]['mail'] = array(
      '#type' => 'hidden',
      '#value' => $req['mail'],
      '#name' => 'mail'
    );
    $form['slots']['audition_mgmt_unassigned_list']['reject_list'][$dq_count]['unreject'] = array(
      '#type' => 'image_button',
      '#name' => 'unreject'.$dq_count,
      '#src' => drupal_get_path('module', 'actorscenter_lottery')."/images/promote.png",
      '#default_value' => $req['uid'],
      '#ajax' => array(
        'callback' => 'lottery_audition_mgmt_submit_callback',
        'wrapper' => 'audition_reject_mgmt_form',
        'method' => 'replace',
        'effect' => 'slide'
      ),
      '#attributes' => array(
        'class' => array('audition_signup_mgmt_unreject', 'audition_signup_mgmt_button'),
        'alt' => "Put this entry back into the pool of eligible auditioners."
      )
    );
  }
}
if(count($unplaced) > 0){
  $form['slots']['audition_mgmt_unassigned_list']['waitlist'] = array(
    '#prefix' => "<div id='audition_waitlist_$nid' class='audition_waitlist'>",
    '#suffix' => "</div>",
    '#tree' => true,
    '#type' => "fieldset",
    '#title' => t("Waitlisted (".count($unplaced).")"),
    '#title_display' => "before"
  );
  foreach ($unplaced as $wl_count => $req){
    $slot_type = "";
    if(!empty($req['slot_type'])){
      $slot_type = " (".$req['slot_type'].")";
    }
    $form['slots']['audition_mgmt_unassigned_list']['waitlist'][$wl_count] = array(
      '#type' => "fieldset",
      '#title_display' => "before",
      '#title' => "<div class='audition_mgmt_table_auditioner_label'><a href='/users/".$req['name']."' target='_blank'>".$req['name']."</a>$slot_type</div>",
      '#attributes' => array(
        'class' => array('audition_mgmt_waitlist_entry', 'waitlist'),
        'id' => array('audition_mgmt_waitlist_entry_'.$nid."_".$wl_count)
      ),
    );
  }
}

$form['slots']['audition_mgmt_table'] = array(
  '#prefix' => "<div class='audition_signup_mgmt_form' id='audition_signup_mgmt_form_$nid'>",
  '#suffix' => "</div>",
  '#title' => t("Current Assignments (".count($placements).")"),
  '#title_display' => "before",
  '#type' => "fieldset",
  '#weight' => 110,
  '#attributes' => array(
    'class' => array('audition_mgmt_table')
  )
  //    '#theme' => 'actorscenter_lottery_components',
);

shuffle($unplaced); // Randomize the starting point for who gets a slot

$interval_s = $interval*60;

$timeblocks = field_view_field('node', $lottery, 'field_lottery_time_blocks');
$tblock_count = 0;
$slot_index = 1;
while(array_key_exists($tblock_count, $timeblocks)){
  $realitem = array_pop($timeblocks[$tblock_count]['entity']['field_collection_item']);
  $start = strtotime($realitem['field_time_range']['#items'][0]['value']);
  $end = strtotime($realitem['field_time_range']['#items'][0]['value2']);
  $group = 0;
  $block_slots = $realitem['field_lottery_audition_block_slo']['#items'][0]['value'];
  $slot_types = array();
  if(array_key_exists('field_restrict_audition_type', $realitem)){
    foreach ($realitem['field_restrict_audition_type']['#items'] as $slottype){
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
    $group_start_dt = new DateTime();
    $group_start_dt->setTimestamp($group_start);
    $group_end_dt = new DateTime();
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
      $form['slots']['audition_mgmt_table'][$group_start][$slot_index] = array(
        '#type' => "fieldset",
        '#attributes' => array(
          'class' => array('audition_mgmt_slot_entry'),
          'id' => array('audition_mgmt_slot_entry_'.$nid."_".$slot_index)
        ),
      );
      $form['slots']['audition_mgmt_table'][$group_start][$slot_index]['uid'] = array(
        '#type' => 'hidden',
        '#name' => 'uid'
      );
      $form['slots']['audition_mgmt_table'][$group_start][$slot_index]['username'] = array(
        '#type' => 'hidden',
        '#name' => 'username'
      );
      $form['slots']['audition_mgmt_table'][$group_start][$slot_index]['mail'] = array(
        '#type' => 'hidden',
        '#name' => 'mail'
      );
      $req = null;
      $saved = false;
      if(array_key_exists($slot_index, $placements)){
        $req = $placements[$slot_index];
        $form['slots']['audition_mgmt_table'][$group_start][$slot_index]['#attributes']['class'][] = "saved";
        $saved = true;
      } elseif(count($unplaced) > 0){
        $req = _actorscenter_lottery_assign_slot($unplaced, $realitem['field_time_range']['#items'][0], $slot_types);
        $form['slots']['audition_mgmt_table'][$group_start][$slot_index]['#attributes']['class'][] = "unsaved";
      }
      $form['slots']['audition_mgmt_table'][$group_start][$slot_index]['#title_display'] = "before";
      if($req != null && $nid && $req['uid']){
        // XXX link to uploaded P/R if they're not a real user
        $form['slots']['audition_mgmt_table'][$group_start][$slot_index]['#title'] = "<div class='audition_mgmt_table_auditioner_label'><a href='/users/".$req['name']."' target='_blank'>".$req['name']."</a></div>";
        $form['slots']['audition_mgmt_table'][$group_start][$slot_index]['uid']['#value'] = $req['uid'];
        $form['slots']['audition_mgmt_table'][$group_start][$slot_index]['mail']['#value'] = $req['mail'];
        $form['slots']['audition_mgmt_table'][$group_start][$slot_index]['username']['#value'] = $req['name'];
        $slot_ajax = array(
          'callback' => 'lottery_audition_mgmt_submit_callback',
          'wrapper' => 'audition_mgmt_slot_entry_'.$nid."_".$slot_index,
          'method' => 'replace',
          'effect' => 'slide'
        );
        $req['placement'] = $slot_index;
        $placements[$slot_index] = $req;
        $form['slots']['audition_mgmt_table'][$group_start][$slot_index]['reject'] = array(
          '#type' => 'image_button',
          '#name' => 'reject'.$slot_index,
          '#src' => drupal_get_path('module', 'actorscenter_lottery')."/images/x.png",
          '#default_value' => $req['uid'],
          '#ajax' => $slot_ajax,
          '#attributes' => array(
            'class' => array('audition_signup_mgmt_reject', 'audition_signup_mgmt_button')
          )
        );
        if(!$saved){
          $form['slots']['audition_mgmt_table'][$group_start][$slot_index]['save'] = array(
            '#type' => 'image_button',
            '#name' => 'save'.$slot_index,
            '#src' => drupal_get_path('module', 'actorscenter_lottery')."/images/save.png",
            '#default_value' => $req['uid'],
            '#ajax' => $slot_ajax,
            '#attributes' => array(
              'class' => array('audition_signup_mgmt_save', 'audition_signup_mgmt_button')
            )
          );
        }

        $form['slots']['audition_mgmt_table'][$group_start][$slot_index]['reload'] = array(
          '#type' => 'image_button',
          '#name' => 'reload'.$slot_index,
          '#src' => drupal_get_path('module', 'actorscenter_lottery')."/images/reload.png",
          '#default_value' => $slot_index,
          '#ajax' => $slot_ajax,
          '#attributes' => array(
            'class' => array('audition_signup_mgmt_reload', 'audition_signup_mgmt_button')
          )
        );

      } else {
        $form['slots']['audition_mgmt_table'][$group_start][$slot_index]['#title'] = "<div class='audition_mgmt_table_auditioner_label'>&lt;available&gt;</div>";
        $form['slots']['audition_mgmt_table'][$group_start][$slot_index]['#attributes']['class'][] = "unassigned";
      }
      $form['slots']['audition_mgmt_table'][$group_start][$slot_index]['weight'] = array(
        '#type' => 'hidden',
        '#delta' => 1000,
        '#default_value' => $slot_index,
        '#title' => t('Weight'),
        '#title_display' => 'invisible',
        '#attributes' => array('class' => array('item-row-weight')),
      );
      $slot_index++;
    }
    $group += $interval_s;
  }
  $tblock_count++;
}

$count = 0;
while($req = $result->fetchAssoc()){
    /*
    $form['slots']['audition_mgmt_table'][$count]['auditioner'] = array(
      '#type' => "item",
      '#markup' => $req['name']." &lt;".$req['mail']."&gt;"
    );
     */
  $count++;
}
return($form);
}


function theme_actorscenter_lottery_components($vars){
  $element = $vars['element'];
  drupal_add_tabledrag($element['#id']."-table", 'order', 'sibling', 'item-row-weight');

  $header = array(
    'auditioner' => t('Auditioner'), 
    //    'slot_type' => t('Slot Type'), 
    'weight' => t('Weight'),
  );

  $rows = array();
  foreach (element_children($element) as $key) {
    $row = array();
    $row['data'] = array();

    foreach ($header as $fieldname => $title) {
      if(!array_key_exists($fieldname, $element[$key])){
        continue;
      }
      $row['data'][] = drupal_render($element[$key][$fieldname]);
      if(!$element[$key]['auditioner']['#tree']){
        //  $row['class'] = array('draggable');
      }
    }
    $rows[] = $row;
  }

  return theme('table', array(
    'header' => $header, 
    'rows' => $rows,
    'attributes' => array('id' => $element['#id']."-table"),
  ));
}

function actorscenter_lottery_manage($nid) {
  _audition_display_bits();
  $lottery = node_load($nid);
  $output = array();

  // Skip auditions that have ended, obviously
  list($start, $end) = _lottery_audition_get_times($lottery);
  if($end < time()){
    $output['#markup'] = "<h1>THIS AUDITION HAS ENDED</h1>".drupal_render(node_view($lottery, "full"));
    return $output;
  }

  // If we have an audition that's not past the signup deadline yet, make a
  // note of it but don't provide the full form.
  $deadline = drupal_render(field_view_field('node', $lottery, 'field_lottery_audition_signup_de'));
  $deadline_str = field_view_field('node', $lottery, 'field_lottery_audition_signup_de');
  $deadline_time = strtotime($deadline_str['#items'][0]['value']);
  if(time() < $deadline_time){
    $output['#markup'] = "<h1>SIGNUPS HAVE NOT YET CLOSED</h1>".drupal_render(node_view($lottery, "full"));
    return $output;
  }

  return(drupal_get_form('audition_mgmt_'.$nid, $nid, $lottery));
}

function _lottery_audition_list_groups($lottery){
  $interval_s = $lottery->field_group_interval[LANGUAGE_NONE][0]['value']*60;
  $timeblocks = field_view_field('node', $lottery, 'field_lottery_time_blocks');
  $tblock_count = 0;
  $groups = array();
  while(array_key_exists($tblock_count, $timeblocks)){
    $realitem = array_pop($timeblocks[$tblock_count]['entity']['field_collection_item']);
    $start = strtotime($realitem['field_time_range']['#items'][0]['value']);
    $end = strtotime($realitem['field_time_range']['#items'][0]['value2']);
    $group = 0;
    $block_slots = $realitem['field_lottery_audition_block_slo']['#items'][0]['value'];
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
      $groups[] = array($group_start, $group_end, $slots_per_group);
      $group += $interval_s;
    }
    $tblock_count++;
  }
  return $groups;
}

function lottery_audition_mgmt_validate_callback($form, &$form_state){
  $lottery = node_load($form_state['values']['lottery_nid']);
  $groups = _lottery_audition_list_groups($lottery);
  return $form;
}
function _lottery_audition_new_form($nid, $form_state){
  $new_state = array();
  $new_state['build_info'] = $form_state['build_info'];
  $new_state['rebuild'] = TRUE;
  $new_state['values'] = array();
  $new_state += form_state_defaults();
  return drupal_rebuild_form('audition_mgmt_'.$nid, $new_state);
}
function lottery_audition_mgmt_submit_callback($form, &$form_state){
  $nid = $form_state['values']['lottery_nid'];
  $lottery = node_load($form_state['values']['lottery_nid']);
  $groups = _lottery_audition_list_groups($lottery);
  switch ($form_state['triggering_element']['#name']) {
  case "save-$nid":
    $slot_index = 1;
    $saved = 0;
    foreach ($groups as $group){
      list($start, $end, $slots) = $group;
      for($i=0; $i < $slots; $i++){
        if(!empty($form_state['values'][$start][$slot_index]['uid'])){
          $result = db_update('lottery_audition_signups')
            ->fields(array('placement' => $slot_index))
            ->condition('nid', $form_state['values']['lottery_nid'])
            ->condition('uid', $form_state['values'][$start][$slot_index]['uid'])
            ->execute();
          $saved++;
        } else {
          db_update('lottery_audition_signups')
            ->fields(array('placement' => null))
            ->condition('nid', $form_state['values']['lottery_nid'])
            ->condition('placement', $slot_index)
            ->execute();
        }
        $slot_index++;
      }
    }
    drupal_set_message("Saved $saved slots (".count($groups)." groups).");
    break;
  case "reassign-$nid":
    // this doesn't actually have to do anything, reloading the form will
    // plant new assignments into unsaved slots
    drupal_set_message("Reassigned all unsaved and empty slots with random selections.");
    break;
  case "unassign-$nid":
    drupal_set_message("Removed all assignments from $lottery->title", "warning");
    db_update('lottery_audition_signups')
      ->fields(array('placement' => null))
      ->condition('nid', $form_state['values']['lottery_nid'])
      ->condition('placement', 1, '>=')
      ->execute();
    break;
  default:
    // These are all AJAX responses.
    $commands = array();
    if(preg_match("/^reject\d+/", $form_state['triggering_element']['#name'])){
      db_update('lottery_audition_signups')
        ->fields(array('placement' => -1))
        ->condition('nid', $form_state['values']['lottery_nid'])
        ->condition('uid', $form_state['triggering_element']['#default_value'])
        ->execute();
      $parents = $form_state['triggering_element']['#array_parents'];
      $group = $parents[count($parents)-3];
      $slot_index = $parents[count($parents)-2];
      drupal_set_message("Removed <strong>".$form_state['values'][$group][$slot_index]['username']."</strong> from eligibility for <strong>".$lottery->title."</strong>", "warning");
      $commands[] = ajax_command_remove('div.messages');
      $commands[] = ajax_command_before('div.region-content', theme('status_messages'));
      $newform = _lottery_audition_new_form($nid, $form_state);
      $commands[] = ajax_command_replace('#audition_signup_mgmt_form_'.$nid, render($newform['slots']['audition_mgmt_table']));
      $commands[] = ajax_command_replace('#audition_reject_mgmt_form_'.$nid, render($newform['slots']['audition_mgmt_unassigned_list']['reject_list']));
      $commands[] = ajax_command_replace('#audition_waitlist_'.$nid, render($newform['slots']['audition_mgmt_unassigned_list']['waitlist']));
      return array('#type' => 'ajax', '#commands' => $commands);

    } elseif(preg_match("/^unreject\d+/", $form_state['triggering_element']['#name'])){
      db_update('lottery_audition_signups')
        ->fields(array('placement' => 0))
        ->condition('nid', $form_state['values']['lottery_nid'])
        ->condition('uid', $form_state['triggering_element']['#default_value'])
        ->execute();
      $parents = $form_state['triggering_element']['#array_parents'];
      $slot_index = $parents[count($parents)-2];
      drupal_set_message("User <strong>".$form_state['values']['reject_list'][$slot_index]['username']."</strong> is now eligible for <strong>".$lottery->title."</strong>");
      $commands[] = ajax_command_remove('div.messages');
      $commands[] = ajax_command_before('div.region-content', theme('status_messages'));
      $newform = _lottery_audition_new_form($nid, $form_state);
      $commands[] = ajax_command_replace('#audition_reject_mgmt_form_'.$nid, render($newform['slots']['audition_mgmt_unassigned_list']['reject_list']));
      $commands[] = ajax_command_replace('#audition_signup_mgmt_form_'.$nid, render($newform['slots']['audition_mgmt_table']));
      $commands[] = ajax_command_replace('#audition_waitlist_'.$nid, render($newform['slots']['audition_mgmt_unassigned_list']['waitlist']));
      return array('#type' => 'ajax', '#commands' => $commands);

    } elseif(preg_match("/^save\d+/", $form_state['triggering_element']['#name'])){
      $parents = $form_state['triggering_element']['#array_parents'];
      $slot_index = $parents[count($parents)-2];
      db_update('lottery_audition_signups')
        ->fields(array('placement' => $slot_index))
        ->condition('nid', $form_state['values']['lottery_nid'])
        ->condition('uid', $form_state['triggering_element']['#default_value'])
        ->execute();
      drupal_set_message("User <strong>".$form_state['values']['reject_list'][$slot_index]['username']."</strong> saved to slot <strong>".$slot_index."</strong>");
      $commands[] = ajax_command_remove('div.messages');
      $commands[] = ajax_command_before('div.region-content', theme('status_messages'));
      $newform = _lottery_audition_new_form($nid, $form_state);
      $commands[] = ajax_command_replace('#audition_signup_mgmt_form_'.$nid, render($newform['slots']['audition_mgmt_table']));
      $commands[] = ajax_command_replace('#audition_waitlist_'.$nid, render($newform['slots']['audition_mgmt_unassigned_list']['waitlist']));
      return array('#type' => 'ajax', '#commands' => $commands);

    } elseif(preg_match("/^reload\d+/", $form_state['triggering_element']['#name'])){
      $parents = $form_state['triggering_element']['#array_parents'];
      $slot_index = $parents[count($parents)-2];
      db_update('lottery_audition_signups')
        ->fields(array('placement' => 0))
        ->condition('nid', $form_state['values']['lottery_nid'])
        ->condition('placement', $form_state['triggering_element']['#default_value'])
        ->execute();
      drupal_set_message("User <strong>".$form_state['values']['reject_list'][$slot_index]['username']."</strong> removed from slot <strong>".$slot_index."</strong> and waitlisted.");
      $commands[] = ajax_command_remove('div.messages');
      $commands[] = ajax_command_before('div.region-content', theme('status_messages'));
      $newform = _lottery_audition_new_form($nid, $form_state);
      $commands[] = ajax_command_replace('#audition_signup_mgmt_form_'.$nid, render($newform['slots']['audition_mgmt_table']));
      $commands[] = ajax_command_replace('#audition_waitlist_'.$nid, render($newform['slots']['audition_mgmt_unassigned_list']['waitlist']));
      return array('#type' => 'ajax', '#commands' => $commands);
    }
    break;
  }
  return(_lottery_audition_new_form($nid, $form_state));
}