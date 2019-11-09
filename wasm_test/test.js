fetch( "test.wasm" ).then( response => 
        response.arrayBuffer()
    ).then( bytes => 
        WebAssembly.compile(bytes)
    ).then( module => 
    {
        var imports= {};
        imports.env= {};
        imports.env.memoryBase = 0;
        imports.env.__linear_memory= new WebAssembly.Memory({ initial: 64 });
        imports.env.cosf= Math.cos;
        var inst= new WebAssembly.Instance( module, imports );
               
        console.log( "exports: " )
        for( e in inst.exports )
            console.log( "    " + e );
        console.log( "end exports" )

        inst.exports.main= function() { console.log("Ü halt"); };
        console.log(inst.exports._Z3Divii(15, 5))
        console.log( "cos: " + inst.exports._Z5MyCosf( 0.01 ) )

        //inst.exports.main();
       // var str_buff = new Uint8Array( inst.exports.memory.buffer, ptr, 64 );
    } );
