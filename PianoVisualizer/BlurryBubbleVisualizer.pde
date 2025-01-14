class BlurryBubbleVisualizer extends Visualizer {
  int blurStateCount = 20;

  PGraphics blurStates[] = new PGraphics[blurStateCount];
  public BlurryBubble[] visualItems = new BlurryBubble[88];

  float size = 100;
  float x, y;
  color ballColor;
  float intensity = 0.01;
  int life = 21;
  boolean setup = false;

  //visualItems[i] = new BlurryBubble(increment*i + (width / keys)/2, height, lerpColor(startColor, endColor, 0.01 * i), screenWidth / keys);

  //  BlurryBubble(float startX, float startY, color myColor, float mySize) {
  BlurryBubbleVisualizer(int[] colorSet) {    
    if (!setup) {
      for (int i=0; i < blurStateCount; i++) {
        blurStates[i] = createGraphics(200, 200, P3D);
        blurStates[i].beginDraw();
        blurStates[i].fill(100);
        ellipseMode(CENTER);
        blurStates[i].ellipse(200/2, 200/2, size, size);
        blurStates[i].filter(BLUR, i+1);
        blurStates[i].endDraw();
      }

      float increment = width / keys;
      color startColor = colorSet[0];
      color endColor = colorSet[1];

      for (int i=0; i < keys; i++) {
        float lerpAmount = (1/keys) * i;
        visualItems[i] = new BlurryBubble(increment*i, height, lerpColor(startColor, endColor, 0.01 * i), screenWidth / keys);
      }
      setup = true;
    }
  }

  void update(int itemId) {
    visualItems[itemId].update(blurStates);
  }
  
  void ping(int itemId) {
    visualItems[itemId].ping();
  }
  
  void pong(int itemId) { }
}

class BlurryBubble extends VisualItem {
  float size = 20;
  float x, y;
  color ballColor;
  float intensity = 0.01;
  float currentLife = 19;
  int maxLife = 19;
  boolean setup = false;

  BlurryBubble(float startX, float startY, color myColor, float mySize) {
    x = startX - 100;
    y = startY;
    ballColor = myColor;
    size = 100;
  }

  void update(PGraphics[] blurStates) { 
    if (currentLife < maxLife) {
       tint(ballColor, 255 - ((255/maxLife) * currentLife));    
       image(blurStates[int(currentLife)], x, y);

      currentLife = currentLife + 0.25;
    }
    
    if (y >= height) {
      y = height;
    } else {
      y = y + (height * intensity);
    }
  }

  void ping() {
    currentLife = 0;

    if (y >= height) {
      y = 0;
    } else {
      y = y - (height * (intensity * 10));
    }
  }
}

