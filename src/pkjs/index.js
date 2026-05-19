Pebble.addEventListener('ready', function(e) {
  console.log('PebbleKit JS ready!');
});

Pebble.addEventListener('appmessage', function(e) {
  if (e.payload.STOP_SOUND !== undefined) {
    console.log('STOP_SOUND message received');
    if (typeof Pebble.stopSound === 'function') {
      Pebble.stopSound();
    }
    return;
  }

  if (e.payload.PLAY_DEFAULT_RINGTONE !== undefined) {
    console.log('PLAY_DEFAULT_RINGTONE message received');
    if (typeof Pebble.playDefaultRingtone === 'function') {
      Pebble.playDefaultRingtone();
      Pebble.sendAppMessage({ 'PLAY_SUCCESS': 1 });
    } else {
      console.log('Pebble.playDefaultRingtone is not available');
      Pebble.sendAppMessage({ 'PLAY_ERROR': 1 });
    }
    return;
  }

  if (e.payload.PLAY_SOUND_BY_ID !== undefined) {
    var rawId = e.payload.PLAY_SOUND_BY_ID;
    var soundId = String(rawId).replace(/\0/g, '').trim();
    console.log('PLAY_SOUND_BY_ID - Raw: "' + rawId + '" Clean: "' + soundId + '"');
    if (typeof Pebble.playSoundById === 'function') {
      Pebble.playSoundById(soundId);
      Pebble.sendAppMessage({ 'PLAY_SUCCESS': 1 });
    } else {
      console.log('Pebble.playSoundById is not available');
      Pebble.sendAppMessage({ 'PLAY_ERROR': 1 });
    }
    return;
  }

  if (e.payload.PLAY_SOUND !== undefined) {
    var rawSoundType = e.payload.PLAY_SOUND;
    // Clean any stray characters from C string
    var soundType = String(rawSoundType).replace(/\0/g, '').trim();
    
    console.log('PLAY_SOUND - Raw: "' + rawSoundType + '" Clean: "' + soundType + '"');
    
    if (typeof Pebble.playSound === 'function') {
      var success = Pebble.playSound(soundType, "Find this phone");
      if (success === false) {
        Pebble.sendAppMessage({ 'PLAY_ERROR': 1 });
      } else {
        Pebble.sendAppMessage({ 'PLAY_SUCCESS': 1 });
      }
    } else {
      console.log('Pebble.playSound is not available in this environment');
      Pebble.sendAppMessage({ 'PLAY_ERROR': 1 });
    }
  }
});
