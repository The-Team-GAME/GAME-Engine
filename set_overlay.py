import json
path='Engine/Assets/levels/MainMenu.json'
with open(path,'r',encoding='utf-8') as f:
    data=json.load(f)

count=0
names=['OptionsWindowed','OptionsFullscreen','OptionsBGMLeft','OptionsBGMRight','OptionsSFXLeft','OptionsSFXRight','OptionsScreenLeft','OptionsScreenRight']
for ent in data['data']['entities']:
    if ent.get('name') in names:
        ent['layers']=['default','overlay_items_1']
        count+=1

with open(path,'w',encoding='utf-8') as f:
    json.dump(data,f,indent=2)

print('updated',count,'entities')
