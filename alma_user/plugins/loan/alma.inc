<?php


/**
 * @file
 * Ding loan plugin.
 */

$plugin = array(
  'description' => t('Alma loan plugin'),
  'version' => 1,
);

/**
 */
function alma_user_loan_get_loans($account, $redirect = FALSE) {
  $creds = ding_library_user_get_credentials($account, $redirect);
  if ($creds == DING_PROVIDER_AUTH_REQUIRED) {
    return $creds;
  }

  try {
    $loans =  alma_client_invoke('get_loans', $creds['user_id'], $creds['password']);
  } catch (Exception $e) {
    watchdog('alma_user', 'get_loans error: “@message”', array('@message' => $e->getMessage()), WATCHDOG_ERROR);
    return FALSE;
  }

  return $loans;
}

function alma_user_loan_details($loan) {
  $cache = &ctools_static(__FUNCTION__, array());
  if (!isset($cache[$loan['id']])) {
    $loan['materials_number'] = $loan['id'];
    if ($object = ting_get_object_by_local_id($loan['record_id'])) {
      $loan['ting_object'] = $object;
      if (ting_object_is($object, 'periodical')) {
        //Loans of periodicals contain information regarding year and volume 
        //on the notes. This should be included in the loan title for the user
        //to see, so we specify a title along with the object
        $loan['title'] = check_plain($object->title).', '.$loan['notes'];
      }
    }
    else {
      $data = alma_client_invoke('catalogue_record_detail', $loan['record_id']);
      $details = $data['records'][$loan['record_id']];
      $loan['title'] = $details['titles'][0];
      $loan['authors'] = (is_array($details['authors'])) ? implode(', ', $details['authors']) : $details['authors'];
      // Remote loans have negative IDs (undocumented Axiell "feature").
      if (intval($loan['id']) < 0) {
        $loan['remote_loan'] = TRUE;
      }
    }
    $cache[$loan['id']] = $loan;
  }
  return $cache[$loan['id']];
}

/**
 */
function alma_user_loan_renew_loans($account, $loan_ids) {
  $creds = ding_library_user_get_credentials($account, $redirect);
  if ($creds == DING_PROVIDER_AUTH_REQUIRED) {
    return $creds;
  }
  try {
    //Return value from Alma client matches Ding provider specification so just return it
    return alma_client_invoke('renew_loan', $creds['user_id'], $creds['password'], $loan_ids);
  }
  catch (Exception $e) {
    watchdog('alma_user', 'renew_loan error: “@message”', array('@message' => $e->getMessage()), WATCHDOG_ERROR);
  }
  return t("Error renewing: internal error");
}
