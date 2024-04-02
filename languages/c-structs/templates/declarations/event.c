/* ${method.name} - ${method.description} */
typedef void (*F${info.Title}${method.Name}Callback)( const void* userData, ${event.signature.callback.params}${if.event.params}, ${end.if.event.params}${method.result.properties} );
int F${info.title}_Register_${method.Name}( ${event.signature.params}${if.event.params}, ${end.if.event.params}F${info.Title}${method.Name}Callback userCB, const void* userData );
int F${info.title}_Unregister_${method.Name}( F${info.Title}${method.Name}Callback userCB);
