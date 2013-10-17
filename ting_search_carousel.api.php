<?php

/**
 * @file
 * Hooks provided by the ting_search_carousel module.
 */

/**
 * @addtogroup hooks
 * @{
 */

/**
 * Get query add-ons from other modules.
 *
 * @param array $query
 * @return array array
 */
function hook_ting_search_carousel_query(&$query) {

  $query['query'] = $query['query'] . $query['foo']

}

/**
 * @} End of "addtogroup hooks".
 */










