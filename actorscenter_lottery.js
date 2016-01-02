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
}(jQuery));
