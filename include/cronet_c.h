// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRONET_NATIVE_INCLUDE_CRONET_C_H_
#define COMPONENTS_CRONET_NATIVE_INCLUDE_CRONET_C_H_

#include "cronet_export.h"

// Cronet public C API is generated from cronet.idl
#include "cronet.idl_c.h"

#ifdef __cplusplus
extern "C" {
#endif

// Stream Engine used by Bidirectional Stream C API for GRPC.

// Additional Cronet C API not generated from cronet.idl.

// Sets net::CertVerifier* raw_mock_cert_verifier for testing of Cronet_Engine.
// Must be called before Cronet_Engine_InitWithParams().
CRONET_EXPORT void Cronet_Engine_SetMockCertVerifierForTesting(
    Cronet_EnginePtr engine,
    /* net::CertVerifier* */ void* raw_mock_cert_verifier);


#ifdef __cplusplus
}
#endif

#endif  // COMPONENTS_CRONET_NATIVE_INCLUDE_CRONET_C_H_
