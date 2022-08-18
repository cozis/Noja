# If-Else statement
An if-else statement lets you specify which portions to code the interpreter must run based on the result of an expression.

The syntax of an if-else statement is the following:
```py
if condition: {
    # Executed when the condition is true
} else {
    # Executed when the condition is false
}
```

The condition may be any type of expression, but must evaluate to a boolean type. No implicit casts are performed. 

When the `else` block is empty, in can me omitted:
```py
if condition: {
    # Executed when the condition is true
}
```

If the blocks only contain one statement, it's possible to omit the curly brackets:
```py
if condition:
    doSomething();
else
    doSomethingElse();
```

## If-else chains
Since curly brackets can be dropped for blocks with only one statement, the following code:
```py
if cond0: {
    doSomething();
} else {
    if cond1: {
        doSomethingElse();
    } else {
        doSomethingDumb();
    }
}
```
can be simplified to
```py
if cond0: {
    doSomething();
} else if cond1: {
    doSomethingElse();
} else {
    doSomethingDumb();
}
```
creating a chain of if-else statements.

## Compound statements
Actually the meaning of the curly brackets is to group multiple statements into one.

The if-else statement expects only one statement for each branch, though it's possible to provide more than one statement each by wrapping them into curly brackets.