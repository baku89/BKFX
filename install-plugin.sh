# Type a script or drag a script file from your workspace to insert its path.

rm -rf "${PLUGIN_DIR}/${FULL_PRODUCT_NAME}"
cp -a -f "${BUILT_PRODUCTS_DIR}/${APPLICATION_NAME}/${FULL_PRODUCT_NAME}" "${PLUGIN_DIR}/${FULL_PRODUCT_NAME}"