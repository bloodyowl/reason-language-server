
open Parsetree;

open Helpers;

/*

Ok, so I'm finding that for this codemod, I want rather more information.
Like, every expression (which I'm not currently doing).

 */

/*

"Convert all Error(x) to Error(Unspecified(x)) if the function's return type is Graphql.Schema.io_field"

 */

let replaceErrors = (ctx, expr) =>
  expr
  ->mapExpr((mapper, expr) => {
      switch (expr.pexp_desc) {
      | Pexp_construct({txt: Longident.Lident("Error")} as lid, Some({pexp_desc: Pexp_tuple([arg])})) =>
        switch (ctx->getExprType(arg)) {
          | Reference(TypeMap.DigTypes.Builtin("string"), []) =>
            Some(
              Ast_helper.Exp.construct(
                lid,
                Some(Ast_helper.Exp.construct(Location.mknoloc(Longident.Lident("Unspecified")), Some(arg))),
              ),
            );
        | _ => None
        }
      | _ => None
      };
    });

let modify = (ctx, structure) => {
  structure->strExpr((mapper, expr) =>
      expr->mapFnExpr((mapper, args, body) => {
          switch (ctx->getExprType(body)) {
          | Reference(Public({moduleName: "Belt", modulePath: ["Result", "t"]}), [_, _]) =>
            Some((args, ctx->replaceErrors(body)))
          | _ => None
          };
        })
      ->Some
    );
};

switch (Sys.argv) {
  | [|_, root|] =>
    print_endline("Running on project: " ++ root);
    Runner.runCodeMod(
      root,
      (path, moduleName) => Filename.extension(path) == ".re",
      modify
    );
  | _ => ()
};
