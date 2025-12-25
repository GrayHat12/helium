# Helium

Just a language for me to understand writting compilers.

## Grammar

![Grammar](./Grammar.md)

## AST Example

```js
// Sample code
let x = 60 + 9 + 2 - 2;
let y = 96;
exit(x);
```

```rs
Program{
    .stmts=[
        Statement{
            .statement=Let{
                .identifier=Token{.type=5, .value=x}, 
                .expression=Expression{
                    .expression=Operation{
                        .left=Expression{
                            .expression=Term{
                                .expression=IntLiteral{
                                    .int_lit=Token{.type=2, .value=60}
                                }
                            }
                        }, 
                        .operator=Token{.type=8, .value=+}, 
                        .right=Expression{
                            .expression=Operation{
                                .left=Expression{
                                    .expression=Term{
                                        .expression=IntLiteral{.int_lit=Token{.type=2, .value=9}}
                                    }
                                }, 
                                .operator=Token{.type=8, .value=+}, 
                                .right=Expression{
                                    .expression=Operation{
                                        .left=Expression{
                                            .expression=Term{
                                                .expression=IntLiteral{.int_lit=Token{.type=2, .value=2}}
                                            }
                                        }, 
                                        .operator=Token{.type=8, .value=-}, 
                                        .right=Expression{
                                            .expression=Term{
                                                .expression=IntLiteral{.int_lit=Token{.type=2, .value=2}}
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }, 
        Statement{
            .statement=Let{
                .identifier=Token{.type=5, .value=y}, 
                .expression=Expression{
                    .expression=Term{
                        .expression=IntLiteral{.int_lit=Token{.type=2, .value=96}}
                    }
                }
            }
        }, 
        Statement{
            .statement=Exit{
                .expression=Expression{
                    .expression=Term{
                        .expression=Identifier{
                            .int_lit=Token{.type=5, .value=x}
                        }
                    }
                }
            }
        }, 
    ]
}
```

## Requirements

* CMake
* [Just](https://github.com/casey/just)

## Build

```sh
just build
```

## Compile helium code

```sh
./build/helium <input.he> <output>
```

## Example

```sh
./build/helium test/test.he out && ./out; echo $?
```