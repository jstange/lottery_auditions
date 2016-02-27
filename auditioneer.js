jQuery(document).ready(function ($) {
  function createCookie(name, value, days) {
    var expires;

    if (days) {
      var date = new Date();
      date.setTime(date.getTime() + (days * 24 * 60 * 60 * 1000));
      expires = "; expires=" + date.toGMTString();
    } else {
      expires = "";
    }
    document.cookie = encodeURIComponent(name) + "=" + encodeURIComponent(value) + expires + "; path=/";
  }

  function readCookie(name) {
    var nameEQ = encodeURIComponent(name) + "=";
    var ca = document.cookie.split(';');
    for (var i = 0; i < ca.length; i++) {
      var c = ca[i];
      while (c.charAt(0) === ' ') c = c.substring(1, c.length);
      if (c.indexOf(nameEQ) === 0) return decodeURIComponent(c.substring(nameEQ.length, c.length));
    }
    return null;
  }

  function eraseCookie(name) {
    createCookie(name, "", -1);
  }
  eraseCookie("auditioneer_pinned");

  $(".auditioneer_audition_timeblock_paint_checked").css("color", Drupal.settings.auditioneer.timeblock_checked_text);
  $(".auditioneer_audition_timeblock_paint_checked").css("background-color", Drupal.settings.auditioneer.timeblock_checked);
  $(".auditioneer_audition_timeblock_paint_unchecked").css("color", Drupal.settings.auditioneer.timeblock_unchecked_text);
  $(".auditioneer_audition_timeblock_paint_unchecked").css("background-color", Drupal.settings.auditioneer.timeblock_unchecked);


  $("div.audition_signup_mgmt_form fieldset.audition_mgmt_slot_entry, div.audition_rejectlist fieldset.audition_mgmt_reject_entry, div.audition_waitlist fieldset.audition_mgmt_waitlist_entry").click(function () {
    is_pinned = readCookie("auditioneer_pinned");
    var me = $(this).find("input.audition_signup_mgmt_button").attr("slot")+$(this).find("input.audition_signup_mgmt_button").attr("name");
    if(is_pinned == null || is_pinned != me){
      $("div.auditioneer_popup_panel").hide();
      $(this).find("div.auditioneer_popup_panel").addClass("pinned");
      $(this).find("div.auditioneer_popup_panel").show();
      $(this).find("div.auditioneer_popup_panel img.auditioneer_pin").show();
      createCookie("auditioneer_pinned", me, 7);
    } else {
      $(this).find("div.auditioneer_popup_panel").removeClass("pinned");
      $(this).find("div.auditioneer_popup_panel img.auditioneer_pin").hide();
      eraseCookie("auditioneer_pinned");
    }
  });

  $("div.auditioneer_popup_panel div.auditioneer_popup_buttons span").click(function(){
    $("div.auditioneer_popup_panel").hide();
  });

  $("div.audition_signup_mgmt_form fieldset.audition_mgmt_slot_entry").mouseover(function () {
    is_pinned = readCookie("auditioneer_pinned");
    var me = $(this).find("input.audition_signup_mgmt_button").attr("slot")+$(this).find("input.audition_signup_mgmt_button").attr("name");
    if($(this).hasClass("saved")){
      $(this).find("div.auditioneer_popup_panel").css("background-color", "PaleGreen");
    } else {
      $(this).find("div.auditioneer_popup_panel").css("background-color", "LightGoldenRodYellow");
    }
    if(is_pinned == null || is_pinned == me){
      $("div.auditioneer_popup_panel").hide();
      $(this).find("div.auditioneer_popup_panel").show();
    }
  });
  $("div.audition_rejectlist fieldset.audition_mgmt_reject_entry").mouseover(function () {
    is_pinned = readCookie("auditioneer_pinned");
    var me = $(this).find("input.audition_signup_mgmt_button").attr("slot")+$(this).find("input.audition_signup_mgmt_button").attr("name");
    $(this).find("div.auditioneer_popup_panel").css("background-color", "LightPink");
    if(is_pinned == null || is_pinned == me){
      $("div.auditioneer_popup_panel").hide();
      $(this).find("div.auditioneer_popup_panel").show();
    }
  });
  $("div.audition_waitlist fieldset.audition_mgmt_waitlist_entry").mouseover(function () {
    is_pinned = readCookie("auditioneer_pinned");
    var me = $(this).find("input.audition_signup_mgmt_button").attr("slot")+$(this).find("input.audition_signup_mgmt_button").attr("name");
    $(this).find("div.auditioneer_popup_panel").css("background-color", "WhiteSmoke");
    if(is_pinned == null || is_pinned == me){
      $("div.auditioneer_popup_panel").hide();
      $(this).find("div.auditioneer_popup_panel").show();
    }
  });

  $("input.audition_signup_cancel").click(function (e) {
    return window.confirm("Are you sure? This will cancel your audition entirely. If the signup deadline has passed, you will not be able to sign up for this audition again.");
  });
  Drupal.behaviors.auditioneer_mgmt_slots = {
    attach: function (context, settings) {
      var events = [];
      $("div.audition_signup_mgmt_form input.audition_signup_mgmt_cancel").each( function( index, element ){
        events.push(jQuery._data(this, "events"));
      });
      jQuery('div.audition_signup_mgmt_form input.audition_signup_mgmt_cancel').mousedown(function() {
        var slot_idx = $(this).attr("slot") - 1;
        if (window.confirm("Are you sure? This will cancel this user's audition entirely. If the signup deadline has passed, they will not be able to sign up for this audition again.")){
          $.each(events[slot_idx].click, function(e) {
            this.handler();
          });
        }
        return false;
      });
    }
  };
  Drupal.behaviors.auditioneer_waitlist_slots = {
    attach: function (context, settings) {
      var events = [];
      $("div.audition_waitlist input.audition_signup_mgmt_cancel").each( function( index, element ){
        events.push(jQuery._data(this, "events"));
      });
      jQuery('div.audition_waitlist input.audition_signup_mgmt_cancel').mousedown(function() {
        var slot_idx = $(this).attr("slot") - 1;
        if (window.confirm("Are you sure? This will cancel this user's audition entirely. If the signup deadline has passed, they will not be able to sign up for this audition again.")){
          $.each(events[slot_idx].click, function(e) {
            this.handler();
          });
        }
        return false;
      });
    }
  };

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
