```rs
// let x = 1 + 2 * 3 + 1;
// let y = x + 5;
// exit(x);

Program{
    .stmts=[
        Statement{
            .statement=Let{
                .identifier=Token{.type=5, .value=x}, 
                .expression=Expression{
                    .expression=Operation{
                        .left=Expression{
                            .expression=Operation{
                                .left=Term{
                                    .term=Term{
                                        .expression=IntLiteral{.int_lit=Token{.type=2, .value=1}}
                                    }
                                }, 
                                .operator=Token{.type=8, .value=+}, 
                                .right=Expression{
                                    .expression=Operation{
                                        .left=Term{
                                            .term=Term{
                                                .expression=IntLiteral{.int_lit=Token{.type=2, .value=2}}
                                            }
                                        }, 
                                        .operator=Token{.type=8, .value=*}, 
                                        .right=Term{
                                            .term=Term{
                                                .expression=IntLiteral{.int_lit=Token{.type=2, .value=3}}
                                            }
                                        }
                                    }
                                }
                            }
                        }, 
                        .operator=Token{.type=8, .value=+}, 
                        .right=Term{
                            .term=Term{
                                .expression=IntLiteral{
                                    .int_lit=Token{.type=2, .value=1}
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
                    .expression=Operation{
                        .left=Term{.term=Term{.expression=Identifier{.int_lit=Token{.type=5, .value=x}}}}, 
                        .operator=Token{.type=8, .value=+}, 
                        .right=Term{
                            .term=Term{
                                .expression=IntLiteral{.int_lit=Token{.type=2, .value=5}}
                            }
                        }
                    }
                }
            }
        }, 
        Statement{
            .statement=Exit{
                .expression=Term{
                    .term=Term{
                        .expression=Identifier{.int_lit=Token{.type=5, .value=x}}
                    }
                }
            }
        }
    ]
}
```