$$
\begin{align}
    [\text{Prog}] &\to [\text{Stmt}]^* \\
    [\text{Scope}] &\to [\{\text{Stmt}]^*\} \\
    [\text{Stmt}] &\to 
    \begin{cases} 
    exit([\text{Expr}]); \\
    let\space\text{ident} = [\text{Expr}]; \\
    \text{if}\space[\text{Expr}][\text{Scope}] \\
    [\text{Scope}] \\
    \end{cases} \\
    [\text{Expr}] &\to 
    \begin{cases}
        [\text{Term}] \\
        [\text{Operation}] \\
    \end{cases} \\
    [\text{Operation}] &\to [\text{Expr}]\space[\text{Operator}]\space[\text{Expr}] \\
    [\text{Operator}] &\to 
    \begin{cases}
        \text{+} & \text{prec}=0 \\
        \text{-} & \text{prec}=0 \\
        \text{/} & \text{prec}=1 \\
        \text{*} & \text{prec}=1 \\
        \text{\%} & \text{prec}=0 \\
    \end{cases} \\
    [\text{Term}] &\to
    \begin{cases}
        \text{int-lit} \\
        \text{ident} \\
        \text{([Expr])} \\
    \end{cases}
\end{align}
$$