(function ($) {
  Drupal.behaviors.tingSearchCarousel = {
    attach: function(context) {
      $('.search-carousel-query .remove').click(function () {
        $(this).parent('tr').remove();
        return false;
      });
    }
  }
})(jQuery);