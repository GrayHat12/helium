$$
\begin{align}
    [\text{Prog}] &\to [\text{Stmt}]^* \\
    [\text{Stmt}] &\to 
    \begin{cases} 
    exit([\text{Expr}]); \\
    let\space\text{ident} = [\text{Expr}];
    \end{cases} \\
    [\text{Expr}] &\to 
    \begin{cases}
        \text{int\_lit} \\
        \text{ident} \\
        [\text{Operation}] \\
    \end{cases} \\
    [\text{Operation}] &\to [\text{Expr}]\space[\text{Operator}]\space[\text{Expr}] \\
    [\text{Operator}] &\to 
    \begin{cases}
        \text{+} \\
        \text{-} \\
        \text{/} \\
        \text{*} \\
        \text{\%} \\
    \end{cases}
\end{align}
$$