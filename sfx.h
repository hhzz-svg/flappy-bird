#ifndef SFX_H
#define SFX_H

// Procedural sound effects. On WebAssembly these go through the browser's Web
// Audio API; on desktop through QAudioSink. Builds without either backend keep
// every call as a no-op.
namespace Sfx {

void setMuted(bool m);
bool muted();
void noteUserGesture();   // browsers keep audio suspended until the first input

void flap();
void point();
void milestone();
void coin();
void hit();
void select();
void thunder();
void buy();
void denied();

}

#endif
