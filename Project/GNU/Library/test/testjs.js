#!/usr/bin/env node

// Helpers
const fs = require('fs')

// Test MediaInfo js WASM module
const MediaInfoModule = require('../MediaInfoWasm.js')({'locateFile': function (url) { return '../' + url }, 'postRun': function () {
    const MI = new MediaInfoModule.MediaInfo()

    var buffer = Uint8Array.from(fs.readFileSync('../../../../Release/Example.ogg'))

    MI.Open_Buffer_Init(buffer.length, 0)
    MI.Open_Buffer_Continue(buffer)
    MI.Open_Buffer_Finalize()

    var size = MI.Get(MediaInfoModule.Stream.General, 0, 'FileSize')

    MI.Close()
    MI.delete

    if (size == '') process.exit(1)
}});
