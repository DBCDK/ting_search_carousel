<?php

  // Get query based on the index parameter.
  $queries = variable_get('ting_carousel_search_queries', array());
  $query = isset($queries[$index]) ? $queries[$index] : FALSE;

  // Initialize cover items array.
  $cover_items = array();

  $max_covers  = variable_get('ting_search_carousel_covers_max', 20);
  $max_queries = variable_get('ting_search_carousel_queries_max', 400);

  $numResults = 40;
  $page = 1;

  // Loop until we have $max_covers covers, or we've reached $max_queries queries.
  while ( ( count($cover_items) < $max_covers ) && ( $page * $numResults < $max_queries ) ) {

    // If a query is defined, do a look-up else do nothing.
    if ( $query ) {

      // Load the ting client.
      if ( !module_load_include('client.inc', 'ting') ) {
        return FALSE;
      }

      // Try to fetch results $numResults at a time.
      $collection = NULL;
      $collection = ting_do_search($query['query'], $page++, $numResults, array('facets' => array(), 'enrich' => TRUE, 'allObjects' => FALSE))

    }

    // Loop over the search results collecting basic object information.
    $local_ids = array();
    $objects = array();
    foreach ($collection->collections as $ting_collection) {
      foreach ($ting_collection->reply->objects as $object) {
        $local_ids[] = $object->localId;
        $objects[$object->localId] = array(
          'id' => $object->id,
          'title' => isset($object->record['dc:title'][''][0]) ? $object->record['dc:title'][''][0] : '',
          'creator' => isset($object->record['dc:creator']['oss:aut'][0]) ? $object->record['dc:creator']['oss:aut'][0] : '',
        );
      }
    }

    // Load helper function form the covers module.
    module_load_include('pages.inc', 'ting_covers');

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
        } elseif ($cover->thumbnailUrl) {
          $source_url = $cover->thumbnailUrl;
        }

        // Download the cover image and get local uri.
        $file = _ting_covers_pages_fetch_image($filename, $source_url);
        $filename = isset($file->uri) ? $file->uri : $filename;
      }

      // Build object with the information collected.
      $item = new stdClass();
      $item->id = $objects[$local_id]['id'];
      $item->title = $objects[$local_id]['title'];
      $item->creator = $objects[$local_id]['creator'];
      $item->image = image_style_url('ting_search_carousel', $filename);

      $cover_items[] = $item;
    }

    $page++;

  }

  // Update the cache with the fetched items.
  _ting_search_carousel_set_cache($index, $cover_items);

