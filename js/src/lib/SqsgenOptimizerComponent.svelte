<script lang="ts">
    import {
        type Content,
        JSONEditor,
        type TextContent,
        Mode,
        type MenuSeparator,
        type MenuItem,
        type RenderMenuContext,
        type OnChangeStatus,
        type ValidationError
    } from 'svelte-jsoneditor'
    import {downloadAsFile} from "$lib/utils.js";
    import {Pane, Splitpanes} from 'svelte-splitpanes';
    import LinearProgress from '@smui/linear-progress';
    import CircularProgress from '@smui/circular-progress';
    import {loadOptimizer, SqsgenOptimizer} from '$lib/optimizer.js'
    import {onMount} from "svelte";

    import {
        faCirclePlay,
        faCircleStop,
        faArrowAltCircleDown,
        faSquareCaretRight,
        faSquareCaretLeft,
        faSquareCaretDown,
        faSquareCaretUp,
        faFile,
        faCircleQuestion
    } from '@fortawesome/free-regular-svg-icons'
    import Dialog, {Title, Content as DialogContent, Actions, InitialFocus} from '@smui/dialog';
    import Button, {Label} from '@smui/button';
    import List, {Item, Graphic, Text} from '@smui/list';
    import Radio from '@smui/radio';
    import * as Ngl from 'ngl';

    type OptimizationState = {
        is: 'idling' | 'running'
        result?: any
        finished?: number,
        working?: number,
        stopRequested?: boolean
    }

    type ResultViewerState = {
        objectiveIndex: number,
        structureIndex: number,
    }

    type ComponentState = {
        jsonEditorRef: JSONEditor | undefined;
        optimizer?: SqsgenOptimizer,
        inputConfig: any | undefined
        optimizationConfig: any | undefined
        optimization: OptimizationState
        ngl?: any
        viewer?: ResultViewerState
        component?: any
    }

    let state = $state({
        optimization: {
            is: 'idling',
        }
    } as ComponentState);
    let content = {
        json: {
            iterations: 1000000,
            structure: {
                species: ['Fe', 'Fe'],
                supercell: [3, 3, 3],
                lattice: [
                    [
                        2.86,
                        0.0,
                        0.0
                    ],
                    [
                        0.0,
                        2.86,
                        0.0
                    ],
                    [
                        0.0,
                        0.0,
                        2.86
                    ]
                ],
                coords: [
                    [
                        0.0,
                        0.0,
                        0.0
                    ],
                    [
                        0.5,
                        0.5,
                        0.5
                    ]
                ]
            },
            composition: [
                {
                    Fe: 45,
                    Al: 9
                }
            ],
            target_objective: 0
        }
    } as Content;


    onMount(async () => {
        state.optimizer = await loadOptimizer();
        state.ngl = await import('ngl')
        openDialogInfo = true;
    })

    let openDialogFileType = $state(false);
    let openDialogInfo = $state(false);
    let fileTypeDownload = $state('cif');
    let multiThreadingAvailable = (typeof SharedArrayBuffer !== 'undefined')
    const loaded = $derived(state.optimizer !== undefined);
    const running = $derived(state.optimization.is === 'running');
    const idling = $derived(state.optimization.is === 'idling');
    const result = $derived(state.optimization.result);
    const stage = $derived(state.ngl ? (new Ngl.Stage('viewport', {
        backgroundColor: 'white',
        cameraType: 'orthographic',
    })) : undefined);

    function refreshEditor() {
        if (state.jsonEditorRef) {
            state.jsonEditorRef.updateProps({
                onRenderMenu: (items: MenuItem[], context: RenderMenuContext) => {
                    return handleRenderMenu(items, context);
                },
            });
        }
        if (state.viewer)
            updateStage(state.viewer);
    }


    function handleRenderMenu(items: MenuItem[], context: RenderMenuContext): MenuItem[] | undefined {

        const separator: MenuSeparator = {
            type: 'separator'
        }


        const buttons: MenuItem[] = [
            {
                type: 'button',
                onClick: () => {
                    if (state.optimizationConfig && idling && state.optimizer) {
                        state.optimization = {
                            is: 'running',
                            finished: 0.0,
                            working: 0.0,
                            stopRequested: false,
                            result: undefined
                        }
                        state.viewer = undefined;
                        const iterations = state.optimizationConfig.iterations;
                        updateStage(undefined);
                        state.optimizer.optimizeAsync(state.optimizationConfig, 0, stats => {
                            const finished = stats.finished / iterations;
                            if (finished > (state.optimization.finished ?? 0.0))
                                state.optimization.finished = finished;
                            state.optimization.working = stats.working / iterations;
                            return state.optimization.stopRequested;
                        }).then((result) => {
                            state.optimization = {
                                is: 'idling',
                                result: result
                            };
                            console.log("Optimization finished:", result);
                            state.viewer = {
                                objectiveIndex: 0,
                                structureIndex: 0,
                            };
                            refreshEditor();
                        }).catch((e) => {
                            console.error("Optimization failed:", e);
                            state.optimization = {
                                is: 'idling',
                                result: undefined
                            };
                            refreshEditor();

                        })
                    } else if (running)
                        state.optimization.stopRequested = true;

                    refreshEditor();
                },
                icon: running ? faCircleStop : faCirclePlay,
                title: running ? 'Stop optimization' : 'Start optimization',
                disabled: state.optimizationConfig === undefined,
            },
            {
                type: 'button',
                onClick: () => {
                    window.open('https://sqsgenerator.readthedocs.io/en/latest/parameters', '_blank');
                },
                icon: faCircleQuestion,
                title: 'Open help',
            }
        ];
        const disableNavButtons = result === undefined || state.viewer === undefined;
        let fileTypeButtonText = undefined;
        switch (fileTypeDownload) {
            case 'cif':
                fileTypeButtonText = 'CIF';
                break;
            case 'vasp':
                fileTypeButtonText = 'VASP';
                break;
            case 'pdb':
                fileTypeButtonText = 'PDB';
                break;
            case 'json-sqsgen':
                fileTypeButtonText = 'sqsgen';
                break;
            case 'json-ase':
                fileTypeButtonText = 'ase';
                break;
            case 'json-pymatgen':
                fileTypeButtonText = 'pymat';
                break;
            default:
                fileTypeButtonText = '???';
        }
        buttons.push(separator,
            {
                type: "button",
                onClick: () => {
                    if (state.viewer) {
                        state.viewer.objectiveIndex -= 1;
                        state.viewer.structureIndex = 0;
                        refreshEditor();
                    }
                },
                icon: faSquareCaretDown,
                title: 'Previous objective',
                disabled: disableNavButtons || state.viewer.objectiveIndex === 0,
            },
            {
                type: "button",
                onClick: () => {
                },
                title: state.viewer ? 'Current objective: ' + result.objective(state.viewer.objectiveIndex) : undefined,
                text: state.viewer && result ? state.viewer.objectiveIndex + 1 + "/" + result.numObjectives() : undefined,
            },
            {
                type: "button",
                onClick: () => {
                    if (state.viewer) {
                        state.viewer.objectiveIndex += 1;
                        state.viewer.structureIndex = 0;
                        refreshEditor();
                    }
                },
                icon: faSquareCaretUp,
                title: 'Next objective',
                disabled: disableNavButtons || state.viewer.objectiveIndex === result.numObjectives() - 1,
            },
            {
                type: "button",
                onClick: () => {
                    if (state.viewer) {
                        state.viewer.structureIndex -= 1;
                        refreshEditor();
                    }
                },
                icon: faSquareCaretLeft,
                title: 'Previous solution',
                disabled: disableNavButtons || state.viewer.structureIndex === 0,
            },
            {
                type: "button",
                onClick: () => {
                },
                text: state.viewer && result ? state.viewer.structureIndex + 1 + "/" + result.numSolutions(state.viewer.objectiveIndex) : undefined,
            },
            {
                type: "button",
                onClick: () => {
                    if (state.viewer) {
                        state.viewer.structureIndex += 1;
                        refreshEditor();
                    }
                },
                icon: faSquareCaretRight,
                title: 'Previous solution',
                disabled: result === undefined || state.viewer === undefined || state.viewer.structureIndex === result.numSolutions(state.viewer.objectiveIndex) - 1,
            },
            separator,
            {
                type: "button",
                onClick: () => {
                    downloadAsFile(result.msgpack(), 'sqs.msgpack', 'application/vnd.msgpack');
                },
                disabled: result === undefined,
                icon: faArrowAltCircleDown,
                title: 'Download results in msgpack format',
            },
            {
                type: "button",
                onClick: () => {
                    if (!result || !state.viewer) return;
                    const stem = `sqsgen-${state.viewer.objectiveIndex}-${state.viewer.structureIndex}`;
                    if (fileTypeDownload == 'cif') {
                        downloadAsFile(result.cif(state.viewer.objectiveIndex, state.viewer.structureIndex), `${stem}.cif`, 'chemical/x-cif');
                    } else if (fileTypeDownload === 'vasp') {
                        downloadAsFile(result.poscar(state.viewer.objectiveIndex, state.viewer.structureIndex), `${stem}.vasp`, 'text/plain');
                    } else if (fileTypeDownload === 'pdb') {
                        downloadAsFile(result.pdb(state.viewer.objectiveIndex, state.viewer.structureIndex), `${stem}.pdb`, 'text/plain');
                    } else if (fileTypeDownload === 'json-sqsgen') {
                        downloadAsFile(result.sqsgen(state.viewer.objectiveIndex, state.viewer.structureIndex), `${stem}.json`, 'application/json');
                    } else if (fileTypeDownload === 'json-ase') {
                        downloadAsFile(result.ase(state.viewer.objectiveIndex, state.viewer.structureIndex), `${stem}-ase.json`, 'application/json');
                    } else if (fileTypeDownload === 'json-pymatgen') {
                        downloadAsFile(result.pymatgen(state.viewer.objectiveIndex, state.viewer.structureIndex), `${stem}-pymat.json`, 'application/json');
                    }
                },
                disabled: result === undefined,
                icon: faFile,
                title: 'Download results in msgpack format',
            },
            {
                type: "button",
                text: fileTypeButtonText,
                onClick: () => {
                    openDialogFileType = true;
                },
                className: "filetype-label-button",
                title: "Change file type",

            },
        )
        const space = {
            type: "space"
        };
        const [textButton, treeButon, ...others] = items;
        treeButon.className = "jse-group-button jse-last";

        const jseHead = [textButton, treeButon, ...others.slice(7, others.length - 1).filter((i) => i.type === 'button')];
        console.log();

        return [
            ...jseHead,
            separator,
            ...buttons,
            space
        ];
    }

    function handleChange(c: TextContent, _: Content, status: OnChangeStatus) {
        state.inputConfig = status.contentErrors ? undefined : JSON.parse(c.text);
        if (state.optimizer) {
            const parsedConfig = state.optimizer.parseConfig(state.inputConfig);
            state.optimizationConfig = parsedConfig.ok ? parsedConfig.value : undefined;
        }

        state.jsonEditorRef?.updateProps(
            {
                onRenderMenu: (items: MenuItem[], context: RenderMenuContext) => {
                    return handleRenderMenu(items, context);
                }
            }
        );
    }

    function validator(json: any): ValidationError[] {
        return state.optimizer ? state.optimizer.validate(json) : []
    }

    async function updateStage(viewerState?: ResultViewerState) {
        if (!stage || !result || !viewerState) return;
        stage.removeAllComponents();
        // 2. Prepare the PDB as a blob
        const pdbText = result.pdb(
            viewerState.objectiveIndex,
            viewerState.structureIndex
        );
        const blob = new Blob([pdbText], {type: "text/plain"});
        const c = await stage.loadFile(blob, {ext: 'pdb'});
        c.addRepresentation("spacefill", {
            radius_type: "vdw",
            color_scheme: 'element',
            radius: 0.5
        });
        c.addRepresentation("unitcell");
        c.autoView();
        c.updateRepresentations({what: {position: true, color: true}});
        stage.viewer.requestRender();

    }

    function closeHandler() {
        refreshEditor();
    }
</script>


<Dialog
        bind:open={openDialogFileType}
        selection
        aria-labelledby="list-selection-title"
        aria-describedby="list-selection-content"
        onSMUIDialogClosed={closeHandler}
>
    <Title id="list-selection-title">Choose download format</Title>
    <DialogContent id="list-selection-content">
        <List radioList>
            <Item use={[InitialFocus]}>
                <Graphic>
                    <Radio bind:group={fileTypeDownload} value="cif"/>
                </Graphic>
                <Text>CIF</Text>
            </Item>
            <Item>
                <Graphic>
                    <Radio bind:group={fileTypeDownload} value="vasp"/>
                </Graphic>
                <Text>VASP (POSCAR)</Text>
            </Item>
            <Item>
                <Graphic>
                    <Radio bind:group={fileTypeDownload} value="pdb"/>
                </Graphic>
                <Text>PDB</Text>
            </Item>
            <Item>
                <Graphic>
                    <Radio bind:group={fileTypeDownload} value="json-sqsgen"/>
                </Graphic>
                <Text>JSON (sqsgen)</Text>
            </Item>
            <Item>
                <Graphic>
                    <Radio bind:group={fileTypeDownload} value="json-ase"/>
                </Graphic>
                <Text>JSON (ase)</Text>
            </Item>
            <Item>
                <Graphic>
                    <Radio bind:group={fileTypeDownload} value="json-pymatgen"/>
                </Graphic>
                <Text>JSON (pymatgen)</Text>
            </Item>
        </List>
    </DialogContent>
    <Actions>
        <Button action="accept">
            <Label>OK</Label>
        </Button>
    </Actions>
</Dialog>


{#if !loaded}
    <div class="loading-pane">
        <div style="display: flex; align-items: center; gap: 16px;">
            <img src="/logo_large.svg" style="width: 175px; height: auto" alt="sqsgen logo"/>
            <CircularProgress style="height: 32px; width: 32px;" indeterminate/>
        </div>
    </div>
{/if}

{#if loaded && multiThreadingAvailable}
    <Dialog
            bind:open={openDialogInfo}
            aria-labelledby="default-focus-title"
            aria-describedby="default-focus-content"
    >
        <Title id="default-focus-title">Affiliation and Templates</Title>
        <DialogContent id="default-focus-content">
            <ul>
                <li>leave a <a href="https://github.com/dgehringer/sqsgenerator">🌟star</a></li>
                <li><strong>Affiliation</strong></li>
                <ul>
                    <li>like the package? Let's add your affiliation to our <a
                            href="https://sqsgenerator.readthedocs.io/en/latest">docs</a></li>
                    <li>send logo and full name via <a
                            href="mailto:35268573+dgehringer@users.noreply.github.com">mail</a></li>
                </ul>
                <li>
                    <strong>Templates</strong></li>
                <ul>
                    <li>want to share an input file with the community?</li>
                    <li>send me the JSON, a name for the template, a list of authors with affiliation, a description the
                        authors and the an eventual DOI in case you have used it in an article via
                        <ul>
                            <li>via <a
                                    href="mailto:35268573+dgehringer@users.noreply.github.com">mail</a></li>
                            <li>by opening <a href="https://github.com/dgehringer/sqsgenerator/issues/new">a new
                                issue</a></li>

                        </ul>
                    </li>
                    <li>it will be packaged into the next release automatically</li>
                    <li>everyone can use it</li>
                </ul>
            </ul>
        </DialogContent>
        <Actions>
            <Button action="accept">
                <Label>OK</Label>
            </Button>
        </Actions>
    </Dialog>
    {#if running}
        <LinearProgress progress={state.optimization.finished} buffer={state.optimization.working}/>
    {/if}
    <Splitpanes vertical={true} style="height: 500px">
        <Pane minSize={30}>
            <JSONEditor bind:this={state.jsonEditorRef} mode={Mode.text} bind:content={content} onChange={handleChange}
                        onRenderMenu={handleRenderMenu} {validator}
            />
        </Pane>
        <Pane size={60} hidden={result === undefined}>
            <div id="viewport" style="height: 100%; width: 100%"></div>
        </Pane>
    </Splitpanes>
{/if}


<style lang="scss">
  .loading-pane {
    display: flex;
    justify-content: center;
    align-items: center;
    height: 100vh;
    width: 100vw;
  }
</style>
