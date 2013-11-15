<?php

// Get query based on the index parameter.
$queries = variable_get('ting_carousel_search_queries', array());
$query = isset($queries[$index]) ? $queries[$index] : FALSE;

// Give other modules an option to alter the carousel query
$query = module_invoke_all('ting_search_carousel_query', $query);

$cover_items = array();

$max_covers = variable_get('ting_search_carousel_covers_max', 20);
$max_queries = variable_get('ting_search_carousel_queries_max', 400);

$numResults = 40;
$start = 1;

// Loop until we have $max_covers covers, or we've reached $max_queries queries.
while ((count($cover_items) < $max_covers) && ($start - $numResults < $max_queries)) {

  // If a query is defined, do a look-up else do nothing.
  if ($query) {

    $params['query'] = $query['query'];

    // results pr. page (opensearch::stepValue) ... in ting-client/lib/request/TingClientSearchRequest.php this is called numResults
    $params['numResults'] = $numResults;

    // page number (opensearch::start)
    $params['start'] = $start; // search results start at position 1

    $params['includeHoldingsCount'] = FALSE;

    // search profile (opensearch::profile)
    $params['profile'] = variable_get('ting_search_profile', FALSE);
    if (!($params['profile'])) {
      watchdog('ting search carousel', 'search profile is not set - yields no search results', array(), WATCHDOG_ERROR, url('admin/config/ting/settings'));
    }

    // agency (opensearch::agency)
    $params['agency'] = variable_get('ting_agency');
    if (!($params['agency'])) {
      watchdog('ting search carousel', 'agency is not set - yields no search results', array(), WATCHDOG_ERROR, url('admin/config/ting/settings'));
    }

    // set Object Format
    $params['objectFormat'] = variable_get('objectFormat', 'briefDisplay');

    //Set queryLanguage
    $params['queryLanguage'] = ting_openformat_get_cql_language();

    if (isset($params['query']) && empty($params['query'])) {
      return array();
    }

    // do the actual search via ting_client_class
    $client = new ting_client_class();

    $result = $client->do_request('search', $params);

    if (!$result) {
      return array();
    }

    // static cache result - used in hook_ding_facetbrowser, ting_search etc.
    drupal_static('ting_search_results', $result);

  }

  $numTotalCollections = $result->numTotalCollections;

  // Loop over the search results collecting basic object information.
  $local_ids = $objects = array();
  /* @var $TingClientObjectCollection TingClientObjectCollection */
  foreach ($result->collections as $key => $TingClientObjectCollection) {
    $manifestations = $TingClientObjectCollection->getFormattedCollection()
      ->getBriefDisplay()->manifestation;
    foreach ($manifestations as $manifestation) {
      $item = array();
      $id = FALSE;
      $item['pid'] = (!empty($manifestation->identifier->{'$'})) ? $manifestation->identifier->{'$'} : FALSE;
      if ($item['pid']) {
        $i = explode(':', $item['pid']);
        $item['lok'] = $i[0];
        $item['lid'] = $id = $i[1];
      }
      $item['title'] = (!empty($manifestation->title->{'$'})) ? $manifestation->title->{'$'} : NULL;
      $item['creator'] = (!empty($manifestation->creator->{'$'})) ? $manifestation->creator->{'$'} : NULL;
      if (!isset($local_ids[$key]) && $id && $item['lok'] == '870970-basis') {
        $local_ids[$key] = $id;
        $objects[$id] = $item;
      }
    }
  }

  // Load helper function form the covers module.
  module_load_include('pages.inc', 'ting_covers');

  // Load helper function from the covers module.
  // module_load_include('pages.inc', 'ting_covers');

  // Get cover service client.
  $service = new AdditionalInformationService(
    variable_get('addi_wsdl_url'),
    variable_get('addi_username'),
    variable_get('addi_group'),
    variable_get('addi_password'));

  // Get covers for the ids.
  $covers = $service->getByFaustNumber($local_ids);

  // Loop over the fetched covers and build items.
  foreach ($covers as $local_id => $cover) {
    // Build cover filename.
    $filename = 'public://ting_search_carousel/' . md5($local_id) . '.jpg';

    // Check if file have been downloaded.
    if (!file_exists($filename)) {
      // Extract the image source url.
      $source_url = FALSE;
      if ($cover->detailUrl) {
        $source_url = $cover->detailUrl;
      }
      elseif ($cover->thumbnailUrl) {
        $source_url = $cover->thumbnailUrl;
      }

      // Download the cover image and get local uri.
      $file = _ting_covers_pages_fetch_image($filename, $source_url);
      $filename = isset($file->uri) ? $file->uri : $filename;
    }

    // Build object with the information collected.
    $item = new stdClass();
    $item->pid = $objects[$local_id]['pid'];
    $item->title = $objects[$local_id]['title'];
    $item->creator = $objects[$local_id]['creator'];
    $item->image = image_style_url('ting_search_carousel', $filename);

    $cover_items[] = $item;
  }

  // Count up search start position
  $start += $numResults;

}

// Update the cache with the fetched items.
_ting_search_carousel_set_cache($index, $cover_items);

