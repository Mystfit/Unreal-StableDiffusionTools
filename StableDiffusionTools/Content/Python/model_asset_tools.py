import requests, re, pathlib, unreal

response = requests.get(url, stream=True, allow_redirects=True)
if response.status_code != 200:
    response.raise_for_status()  # Will only raise for 4xx codes, so...
    print(f"Request to {url} returned status code {r.status_code}")
    downloader.fail_game_thread(0)
file_size = int(response.headers.get('Content-Length', 0))
desc = "(Unknown total file size)" if file_size == 0 else ""

url_filename = response.headers.get('content-disposition')
filenames = re.findall('filename=(.+)', url_filename) 
if filenames:
    filename = filenames[0].replace('\"', '')
    filepath = pathlib.Path(destination_dir) / filename
    print(f"Downloading file to {filepath}")
  
    bytes_total = 0
    with filepath.open("wb") as f:
        #shutil.copyfileobj(r_raw, f)
        with unreal.ScopedSlowTask(file_size, 'Downloading model') as slow_task:
            slow_task.make_dialog(True)
            for chunk in response.iter_content(1024):
                bytes_total += len(chunk)
                f.write(chunk)
                f.flush()
                downloader.update_game_thread(bytes_total)
                slow_task.enter_progress_frame(bytes_total, "Downloading")
            downloader.success_game_thread(file_size)
