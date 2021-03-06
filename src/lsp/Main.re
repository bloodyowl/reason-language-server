
open Infix;
open Result;
open Log;

let capabilities =
  Rpc.J.(
    o([
      ("textDocumentSync", i(1)),
      ("hoverProvider", t),
      ("completionProvider", o([
        ("resolveProvider", t),
        /* TODO list # as trigger character */
        ("triggerCharacters", l([s(".")]))
      ])),
      ("signatureHelpProvider", o([
        ("triggerCharacters", l([s("(")]))
      ])),
      ("definitionProvider", t),
      ("typeDefinitionProvider", t),
      ("referencesProvider", t),
      ("documentSymbolProvider", t),
      /* ("codeActionProvider", t), */
      ("codeLensProvider", o([
        ("resolveProvider", t)
      ])),
      ("documentHighlightProvider", t),
      ("documentRangeFormattingProvider", t),
      ("documentFormattingProvider", t),
      /*
       * Found how to do the showReferences thing
       * https://github.com/Microsoft/vscode/blob/c6b1114292288e76e2901e05e860faf3a08b4b5a/extensions/typescript-language-features/src/features/implementationsCodeLensProvider.ts
       * but it seems I need to instantiate the object from javascript
       */
      /* ("executeCommandProvider", o([
      ])) */
      /* ("executeCommandOptions", t), */
      ("documentFormattingProvider", t),
      ("renameProvider", t)
    ])
);

let getInitialState = (params) => {
  let uri = Json.get("rootUri", params) |?> Json.string |! "Must have a rootUri";
  let%try rootPath = uri |> Utils.parseUri |> resultOfOption("No root uri");

  Files.mkdirp(rootPath /+ "node_modules" /+ ".lsp");
  Log.setLocation(rootPath /+ "node_modules" /+ ".lsp" /+ "debug.log");
  Log.log("Hello from " ++ Sys.executable_name);
  Log.log("Previous log location: " ++ Log.initial_dest);

  Rpc.sendNotification(
    Log.log,
    stdout,
    "client/registerCapability",
    Rpc.J.(
      o([
        (
          "registrations",
          l([
            o([
              ("id", s("watching")),
              ("method", s("workspace/didChangeWatchedFiles")),
              (
                "registerOptions",
                o([
                  (
                    "watchers",
                    l([
                      o([
                        ("globPattern", s("**/bsconfig.json")),
                        ("globPattern", s("**/.merlin")),
                      ]),
                    ]),
                  ),
                ]),
              ),
            ]),
          ]),
        ),
      ])
    ),
  );

  open InfixResult;

  let packagesByRoot = Hashtbl.create(1);

  /* if client needs plain text in any place, we disable markdown everywhere */
  let clientNeedsPlainText = ! Infix.(
      Json.getPath("capabilities.textDocument.hover.contentFormat", params) |?> Protocol.hasMarkdownCap |? true
      && Json.getPath("capabilities.textDocument.completion.completionItem.documentationFormat", params) |?> Protocol.hasMarkdownCap |? true,
  );

  let state = {
    ...TopTypes.empty(),
    rootPath,
    rootUri: uri,
  };

  Ok({...state, settings: {...state.settings, clientNeedsPlainText}})
};

open TopTypes;

let tick = state => {
  NotificationHandlers.checkPackageTimers(state);
  Diagnostics.checkDocumentTimers(state);
};

let main = () => {
  log("Booting up");
  BasicServer.run(
    ~tick,
    ~log,
    ~messageHandlers=MessageHandlers.handlers,
    ~notificationHandlers=NotificationHandlers.notificationHandlers,
    ~capabilities=_params => capabilities,
    ~getInitialState
  );
  log("Finished");
  out^ |?< close_out;
};