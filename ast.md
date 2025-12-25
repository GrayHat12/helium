```rs
// let x = 0;
// if x {
//     exit(69);
// } else {
//     exit(42);
// }

Program{
    .stmts=[
        Statement{
            .statement=Let{
                .identifier=Token{.type=5, .value=x}, 
                .expression=Term{
                    .term=Term{
                        .expression=IntLiteral{.int_lit=Token{.type=2, .value=0}}
                    }
                }
            }
        }, 
        Statement{
            .statement=If{
                .expression=Term{.term=Term{.expression=Identifier{.int_lit=Token{.type=5, .value=x}}}},
                .scope=[
                    Statement{
                        .statement=Exit{
                            .expression=Term{
                                .term=Term{.expression=IntLiteral{.int_lit=Token{.type=2, .value=69}}}
                            }
                        }
                    }
                ],
                .else=Scope{
                    .stmts=[
                        Statement{
                            .statement=Exit{
                                .expression=Term{.term=Term{.expression=IntLiteral{.int_lit=Token{.type=2, .value=42}}}}
                            }
                        }
                    ]
                }
            }
        }
    ]
}
```