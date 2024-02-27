#pragma once

typedef struct lambda_params {
  double md;     //!< Mode decision Lambda
  double me[3];  //!< Motion Estimation Lambda
  int    mf[3];  //!< Integer formatted Motion Estimation Lambda
  } LambdaParams;
