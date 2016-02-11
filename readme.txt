Project 2D README file
================================================================
Author:
Hanjing Fang  Email: hjfang@ucla.edu
Yujing Zhang  Email: yujingzhang@g.ucla.edu

Optimization to reduce page read:
1.Avoid opening table when the select action is only on key or count(*).
2.We set a startCursor and an endCursor to get the range of number we want. Avoid reading values when comparing.

Please feel free to contact us if you have any question.
