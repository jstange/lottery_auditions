<?php

/**
 * @file
 * This template is used to print a single field in a view.
 *
 * It is not actually used in default Views, as this is registered as a theme
 * function which has better performance. For single overrides, the template is
 * perfectly okay.
 *
 * Variables available:
 * - $view: The view object
 * - $field: The field handler object that can process the input
 * - $row: The raw SQL result that can be used
 * - $output: The processed output that will normally be used.
 *
 * When fetching output from the $row, this construct should be used:
 * $data = $row->{$field->field_alias}
 *
 * The above will guarantee that you'll always get the correct data,
 * regardless of any changes in the aliasing that might happen if
 * the view is modified.
 */
?>
<?php
$start = $end = null;
foreach ($row->field_field_lottery_time_blocks as $delta => $item){
  $realitem = array_pop($item['rendered']['entity']['field_collection_item']);
  $cur_start = strtotime($realitem['field_time_range']['#items'][0]['value']);
  $cur_end = strtotime($realitem['field_time_range']['#items'][0]['value2']);
  if ($start == null || $cur_start < $start){
    $start = $cur_start;
  }
  if ($end == null || $cur_end > $end){
    $end = $cur_end;
  }
}
print date("l F dS, Y", $start)." - ".date("l F dS, Y", $end);
?>
