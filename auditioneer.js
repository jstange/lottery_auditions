jQuery(document).ready(function ($) {
  $(".auditioneer_audition_timeblock_paint_checked").css("color", Drupal.settings.auditioneer.timeblock_checked_text);
  $(".auditioneer_audition_timeblock_paint_checked").css("background-color", Drupal.settings.auditioneer.timeblock_checked);
  $(".auditioneer_audition_timeblock_paint_unchecked").css("color", Drupal.settings.auditioneer.timeblock_unchecked_text);
  $(".auditioneer_audition_timeblock_paint_unchecked").css("background-color", Drupal.settings.auditioneer.timeblock_unchecked);

  $(".auditioneer_audition_timeblock_paint").click(function () {
    if(!$(this).hasClass("auditioneer_audition_timeblock_paint_disabled")){
      if($(this).hasClass("auditioneer_audition_timeblock_paint_unchecked")){
        $(this).removeClass("auditioneer_audition_timeblock_paint_unchecked");
        $(this).addClass("auditioneer_audition_timeblock_paint_checked");
        $(this).css("color", Drupal.settings.auditioneer.timeblock_checked_text);
        $(this).css("background-color", Drupal.settings.auditioneer.timeblock_checked);
        $(this).parent().find(".auditioneer_audition_timeblock_checkbox").prop("checked", true);
        $($(this).attr("req_contact_fields")).each( function( index, element ){
          $(this).find("label span.form-required").removeClass("hidden");
        });
      } else {
        $(this).removeClass("auditioneer_audition_timeblock_paint_checked");
        $(this).addClass("auditioneer_audition_timeblock_paint_unchecked");
        $(this).css("color", Drupal.settings.auditioneer.timeblock_unchecked_text);
        $(this).css("background-color", Drupal.settings.auditioneer.timeblock_unchecked);
        $(this).parent().find(".auditioneer_audition_timeblock_checkbox").prop("checked", false);
        $($(this).attr("req_contact_fields")).each( function( index, element ){
          $(this).find("label span.form-required").addClass("hidden");
        });
      }
    }
  });
  $(".auditioneer_audition_timeblock_checkbox").change(function() {
    var paintblock = $(this).parent().parent().find(".auditioneer_audition_timeblock_paint");
    if($(this).is(':checked')){
      paintblock.removeClass("auditioneer_audition_timeblock_paint_unchecked");
      paintblock.addClass("auditioneer_audition_timeblock_paint_checked");
      paintblock.css("color", Drupal.settings.auditioneer.timeblock_checked_text);
      paintblock.css("background-color", Drupal.settings.auditioneer.timeblock_checked);
      $($(this).attr("req_contact_fields")).each( function( index, element ){
        $(this).find("label span.form-required").removeClass("hidden");
      });
    } else {
      paintblock.removeClass("auditioneer_audition_timeblock_paint_checked");
      paintblock.addClass("auditioneer_audition_timeblock_paint_unchecked");
      paintblock.css("color", Drupal.settings.auditioneer.timeblock_unchecked_text);
      paintblock.css("background-color", Drupal.settings.auditioneer.timeblock_unchecked);
      $($(this).attr("req_contact_fields")).each( function( index, element ){
        $(this).find("label span.form-required").addClass("hidden");
      });
    }
  });
  $(".auditioneer_audition_slot_type_option").change(function() {
    pattern = new RegExp('[^a-z]', 'g');
    var classname = $(this).attr("name")+"_"+$(this).val().toLowerCase().replace(pattern, "_");
    var paintblocks = "div."+$(this).attr("name")+"_paint";
    var checkboxes = "input."+$(this).attr("name")+"_checkbox";
    if(classname != "undefined_"){
      if($(this).val() == "General"){
        $(paintblocks).removeClass("auditioneer_audition_timeblock_paint_disabled");
        $(checkboxes).removeAttr("disabled");
      } else {
        $(paintblocks).each( function( index, element ){
          if(!$(this).hasClass(classname)){
            $(this).addClass("auditioneer_audition_timeblock_paint_disabled");
            $(this).removeClass("auditioneer_audition_timeblock_paint_checked");
            $(this).addClass("auditioneer_audition_timeblock_paint_unchecked");
            $(this).css("color", Drupal.settings.auditioneer.timeblock_unchecked_text);
            $(this).css("background-color", Drupal.settings.auditioneer.timeblock_unchecked);
          } else {
            $(this).removeClass("auditioneer_audition_timeblock_paint_disabled");
          }
        });
        $(checkboxes).each( function( index, element ){
          if(!$(this).hasClass(classname)){
            $(this).prop("checked", false);
            $(this).attr("disabled", "disabled");
            $($(this).attr("req_contact_fields")).each( function( index, element ){
              $(this).find("label span.form-required").addClass("hidden");
            });
          } else {
            $(this).removeAttr("disabled");
          }
        });
      }
    }
  });

  function auditioneer_fix_list_height(){
    $("div.audition_signup_mgmt_form").each( function( index, element ){
      $(this).siblings("div.audition_mgmt_unassigned_list").height($(this).outerHeight());
      $(this).siblings("div.audition_mgmt_unassigned_list").css("height", $(this).outerHeight());
    });
  }
  /*
    $('div.form-item-interval select').change(function() {
      $(this).closest('form').submit();
    });
   */
  auditioneer_fix_list_height();
  $(document).ajaxStop(function(){
    auditioneer_fix_list_height();
  });
}(jQuery));
