import json
path='Engine/Assets/levels/MainMenu.json'
with open(path,'r', encoding='utf-8') as f:
    data=json.load(f)

names=['OptionsBGMLeft','OptionsBGMRight','OptionsSFXLeft','OptionsSFXRight','OptionsScreenLeft','OptionsScreenRight']
count=0
for ent in data['data']['entities']:
    if ent.get('name') in names:
        for comp in ent.get('components',[]):
            if comp.get('type')=='Components::Transform2D':
                comp['data']['scale']['x']=29.166666666666666
                comp['data']['scale']['y']=47.5
                count+=1
            if comp.get('type')=='Components::BoxCollider':
                comp['data']['width']=29.166666666666666
                comp['data']['height']=47.5
                comp['data']['aabb']['half_width']=29.166666666666666/2
                comp['data']['aabb']['half_height']=47.5/2

with open(path,'w', encoding='utf-8') as f:
    json.dump(data,f,indent=2)
print('updated',count,'transform entries')
