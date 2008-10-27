<?php
// $Id: search.admin.inc,v 1.4 2008/01/08 10:35:42 goba Exp $

/**
 * @file
 * Admin page callbacks for the search module.
 */

/**
 * Menu callback: confirm wiping of the index.
 */
function tsearch_wipe_confirm() {
  return confirm_form(array(), t('Are you sure you want to re-index the site?'),
                  'admin/settings/search', t(' The search index is not cleared but systematically updated to reflect the new settings. Searching will continue to work but new content won\'t be indexed until all existing content has been re-indexed. This action cannot be undone.'), t('Re-index site'), t('Cancel'));
}

/**
 * Handler for wipe confirmation
 */
function tsearch_wipe_confirm_submit(&$form, &$form_state) {
  if ($form['confirm']) {
    tsearch_wipe();
    drupal_set_message(t('The index will be rebuilt.'));
    $form_state['redirect'] = 'admin/settings/search';
    return;
  }
}

/**
 * Menu callback; displays the search module settings page.
 *
 * @ingroup forms
 * @see system_settings_form()
 * @see tsearch_admin_settings_validate()
 */
function tsearch_admin_settings() {
  // Collect some stats
  $sql = <<<SQL
SELECT count(n.nid)
FROM node AS n
LEFT JOIN tsearch_node AS t ON (t.nid = n.nid)
WHERE ((
    n.changed > t.updated
  )
  OR
  (
    t.updated IS NULL
  )
);
SQL;

  $remaining = db_result(db_query($sql));
  $total = db_result(db_query("SELECT count(nid) FROM {node}"));

  $count = format_plural($remaining, 'There is 1 item left to index.', 'There are @count items left to index.');
  $percentage = ((int)min(100, 100 * ($total - $remaining) / max(1, $total))) .'%';
  $status = '<p><strong>'. t('%percentage of the site has been indexed.', array('%percentage' => $percentage)) .' '. $count .'</strong></p>';
  $form['status'] = array('#type' => 'fieldset', '#title' => t('Indexing status'));
  $form['status']['status'] = array('#value' => $status);
  $form['status']['wipe'] = array('#type' => 'submit', '#value' => t('Re-index site'));

  $items = drupal_map_assoc(array(10, 20, 50, 100, 200, 500));

  // Indexing throttle:
  $form['indexing_throttle'] = array('#type' => 'fieldset', '#title' => t('Indexing throttle'));
  $form['indexing_throttle']['tsearch_cron_limit'] = array('#type' => 'select', '#title' => t('Number of items to index per cron run'), '#default_value' => variable_get('tsearch_cron_limit', 100), '#options' => $items, '#description' => t('The maximum number of items indexed in each pass of a <a href="@cron">cron maintenance task</a>. If necessary, reduce the number of items to prevent timeouts and memory errors while indexing.', array('@cron' => url('admin/reports/status'))));

  $form['#validate'] = array('tsearch_admin_settings_validate');

  // Per module settings
  $form = array_merge($form, module_invoke_all('tsearch', 'admin'));
  return system_settings_form($form);
}

/**
 * Validate callback.
 */
function tsearch_admin_settings_validate($form, &$form_state) {
  if ($form_state['values']['op'] == t('Re-index site')) {
    drupal_goto('admin/settings/tsearch/wipe');
  }
}
