#include "drawingstatefilter.h"
#include <cassert>
#include <iostream>
int main() {
    DrawingStateFilter f;
    bool changed;
    // 25 Hz OFF przez 2s
    for (int i=0;i<50;i++) {
      changed=f.addSample(false, i * 40000LL);
      assert(!changed && !f.state());
    }
    // pojedyncze zaklocenie ON nie zmienia stanu
    changed=f.addSample(true, 50*40000LL);
    assert(!changed && !f.state());
    for(int i=51;i<60;i++) {
      changed=f.addSample(false, i * 40000LL);
      assert(!changed && !f.state());
    }
    // stabilny ON po toggle ESP
    int transitionOn=-1;
    for(int i=60;i<85;i++) {
      changed=f.addSample(true, i * 40000LL);
      if (changed) {transitionOn=i; assert(f.state()); break;}
    }
    assert(transitionOn>=68 && transitionOn<=74);
    // po ustabilizowaniu dalej 1 nie powinno przelaczac na OFF
    for(int i=transitionOn+1;i<110;i++){
       changed=f.addSample(true, i*40000LL);
       assert(!changed && f.state());
    }
    // stabilny OFF
    int transitionOff=-1;
    for(int i=110;i<140;i++){
       changed=f.addSample(false,i*40000LL);
       if(changed){transitionOff=i; assert(!f.state());break;}
    }
    assert(transitionOff>=118 && transitionOff<=124);
    f.reset();
    assert(!f.state());
    std::cout << "TEST OK: ON at " << transitionOn << ", OFF at " << transitionOff << "\n";
}
