/*
 * Copyright  2005 PB Consult Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */
package com.pb.common.calculator2;

import java.io.Serializable;

/**
 * Defines numberic constants for variable types.
 *
 * @author    Tim Heier
 * @version   1.0, 2/22/2003
 */
public final class VariableType implements Serializable {

    public static final int SCALAR            = 1;
    public static final int TABLE             = 2;
    public static final int MATRIX            = 3;
    public static final int MATRIX_COLLECTION = 4;
    public static final int OBJECT            = 5;
    public static final int INTERNAL          = 6;
    public static final int ALTERNATIVE       = 7;

}
