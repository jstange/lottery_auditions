jQuery(document).ready(function ($) {
  $(".lottery_audition_timeblock_paint").click(function () {
    if(!$(this).hasClass("lottery_audition_timeblock_paint_disabled")){
      if($(this).hasClass("lottery_audition_timeblock_paint_unchecked")){
        $(this).removeClass("lottery_audition_timeblock_paint_unchecked");
        $(this).addClass("lottery_audition_timeblock_paint_checked");
        $(this).parent().find(".lottery_audition_timeblock_checkbox").prop("checked", true);
      } else {
  $(this).removeClass("lottery_audition_timeblock_paint_checked");
        $(this).addClass("lottery_audition_timeblock_paint_unchecked");
        $(this).parent().find(".lottery_audition_timeblock_checkbox").prop("checked", false);
      }
    }
  });
  $(".lottery_audition_timeblock_checkbox").change(function() {
    var paintblock = $(this).parent().parent().find(".lottery_audition_timeblock_paint");
    if($(this).is(':checked')){
      paintblock.removeClass("lottery_audition_timeblock_paint_unchecked");
      paintblock.addClass("lottery_audition_timeblock_paint_checked");
    } else {
      paintblock.removeClass("lottery_audition_timeblock_paint_checked");
      paintblock.addClass("lottery_audition_timeblock_paint_unchecked");
    }
  });
  $(".lottery_audition_slot_type_option").change(function() {
    pattern = new RegExp('[^a-z]', 'g');
    var classname = $(this).attr("name")+"_"+$(this).val().toLowerCase().replace(pattern, "_");
    var paintblocks = "div."+$(this).attr("name")+"_paint";
    var checkboxes = "input."+$(this).attr("name")+"_checkbox";
    if(classname != "undefined_"){
      if($(this).val() == "General"){
  $(paintblocks).removeClass("lottery_audition_timeblock_paint_disabled");
  $(checkboxes).removeAttr("disabled");
      } else {
  $(paintblocks).each( function( index, element ){
    if(!$(this).hasClass(classname)){
      $(this).addClass("lottery_audition_timeblock_paint_disabled");
      $(this).removeClass("lottery_audition_timeblock_paint_checked");
      $(this).addClass("lottery_audition_timeblock_paint_unchecked");
    } else {
      $(this).removeClass("lottery_audition_timeblock_paint_disabled");
    }
  });
  $(checkboxes).each( function( index, element ){
    if(!$(this).hasClass(classname)){
      $(this).prop("checked", false);
      $(this).attr("disabled", "disabled");
    } else {
      $(this).removeAttr("disabled");
    }
        });
      }
    }
  });
  // XXX catch WINCH events however you do that
  $("div.audition_signup_mgmt_form").each( function( index, element ){
    $(this).siblings("div.audition_mgmt_unassigned_list").height($(this).outerHeight());
    $(this).siblings("div.audition_mgmt_unassigned_list").css("height", $(this).outerHeight());
  });
}(jQuery));
