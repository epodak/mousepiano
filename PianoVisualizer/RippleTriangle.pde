class RippleTriangle extends VisualItem {
  color triangleColor;
  float x, y;
  float p1x, p1y, p2x, p2y, p3x, p3y;
  int keyIndex;
  int yLine = 0;
  float boxWidth;
  boolean setupPosition = false;
  int intensity = 0;
  float decayRate = 50;

  RippleTriangle(int index, color myColor) {
    keyIndex = index;
    triangleColor = myColor;

    boxWidth = 160;
  }

  void update() {
    boolean flipped = true;
    setPositionForIndex(keyIndex);
    colorMode(RGB, 255);
    fill(triangleColor, intensity);
    noStroke();

    if (keyIndex % 2 == 0) {
      flipped = true;
    } else {
      flipped = false;
    }

    if (yLine % 2 == 0) {
      flipped = !flipped;
    }

    if (flipped) {
      // Even
      p1x = x;
      p1y = y - boxWidth/2;
      p2x = x + boxWidth;
      p2y = y - boxWidth;
      p3x = x + boxWidth;
      p3y = y;
    } else {
      // Odd
      p1x = x;
      p1y = y - boxWidth;
      p2x = x;
      p2y = y;
      p3x = x + boxWidth;
      p3y = y - boxWidth/2;
    }

    triangle(p1x, p1y, p2x, p2y, p3x, p3y);

    if (intensity <= 0) {
      intensity = 0;
    } else {
      intensity -= decayRate;
    }
  }

  void ping() {
    intensity = 255;
  }

  void setPositionForIndex(int index) {
    float xPosition = index * boxWidth;

    while (!setupPosition) {
      if (xPosition < width) {
        x = xPosition;
        y = (yLine * boxWidth/2) + (boxWidth/2);
        setupPosition = true;
      } else {
        xPosition -= width;
        yLine++;
      }
    }

    //println(index+": "+x+", "+y);
  }
}

