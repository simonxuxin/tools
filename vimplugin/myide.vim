" Global Variables {{{
if exists("g:xxide_split_height") == 0
    let g:xxide_split_height = 10
endif

let g:script_languages = {
    \ 'perl': ['perl'], 
    \ 'python': ['python'], 
    \ 'sh': ['sh', 'bash', 'ksh'],
    \}

" }}} 
"
" Main Entry {{{
function! XxIDE_Execute()
    let ft = &filetype
    if ft == ""
        echo "Unknow Filetype"
    elseif ft == "vim"
        call XxIDE_VimExecute()
    else
        call XxIDE_ExternalExecute(ft)
    endif
endfunction
" }}}
"
" External Execution {{{
function! XxIDE_ExternalExecute(ft)
    let fn = expand("%:p")
    let execCmd = XxIDE_GetExecuteString(a:ft, fn)

    if execCmd == ""
        return
    endif

    set splitbelow
    silent execute g:Xxide_split_height . "new"
    silent execute "read " . execCmd
    1d
    setlocal nomodifiable nomodified
endfunction
" }}}
"
" Vim Execution {{{
function! XxIDE_VimExecute()
    source %
endfunction
" }}}
"
" Get Execution Command Line {{{ 
function! XxIDE_GetExecuteString(ft, fn)
    let args = []
    if index(keys(g:script_languages), a:ft) != -1
        if(len(g:script_languages[a:ft]) == 1)
            let cmd = g:script_languages[a:ft][0]
        else
            if exists("w:shcmd")
                let cmd = w:shcmd
            endif
            while exists("cmd") == 0
                let cmd = input("Select Shell (" . join(g:script_languages[a:ft], "/") . ") ")
                if index(g:script_languages[a:ft], cmd) == -1
                    unlet cmd
                else
                    let w:shcmd = cmd
                endif
            endwhile
        endif
        call extend(args, ["!", cmd, a:fn])
        call extend(args, XxIDE_GetExecuteParams())
    endif 
    return join(args, " ")    
endfunction
" }}}
"
" Get Execution Parameters {{{
function! XxIDE_GetExecuteParams()
    let params = input("Any Parameters? ")
    return split(params, " ")
endfunction
" }}}
"
" Add Map {{{
nmap <F5> <C-W><C-O>:call XxIDE_Execute()<CR>
" }}}
